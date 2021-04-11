#include "node.h"
#include "minmax.h"
#include "selector.h"
#include "vtscreen.h"
#include "vthandler.h"
#include <curses.h>

std::shared_ptr<NODE> root;
std::weak_ptr<NODE> focused;
std::weak_ptr<NODE> lastfocused;

NODE::NODE(Node t, const std::shared_ptr<NODE> &p, int y, int x, int h, int w)
{
    this->t = t;
    this->p = p;
    this->y = y;
    this->x = x;
    this->h = h;
    this->w = w;
}

NODE::~NODE()
{
    if (auto l = lastfocused.lock())
        if (l.get() == this)
            lastfocused.reset();
}

void NODE::reshape(int y, int x, int h, int w) /* Reshape a node. */
{
    if (this->y == y && this->x == x && this->h == h && this->w == w &&
        this->vt)
        return;

    int d = this->h - h;
    int ow = this->w;
    this->y = y;
    this->x = x;
    this->h = MAX(h, 1);
    this->w = MAX(w, 1);

    if (this->vt)
        this->vt->reshapeview(d, ow, this->h, this->w);
    else
        reshapechildren();
    this->draw();
}

void NODE::reshapechildren() /* Reshape all children of a view. */
{
    if (this->t == HORIZONTAL)
    {
        int i = this->w % 2 ? 0 : 1;
        this->c1->reshape(this->y, this->x, this->h, this->w / 2);
        this->c2->reshape(this->y, this->x + this->w / 2 + 1, this->h,
                          this->w / 2 - i);
    }
    else if (this->t == VERTICAL)
    {
        int i = this->h % 2 ? 0 : 1;
        this->c1->reshape(this->y, this->x, this->h / 2, this->w);
        this->c2->reshape(this->y + this->h / 2 + 1, this->x, this->h / 2 - i,
                          this->w);
    }
}

void NODE::draw() const /* Draw a node. */
{
    if (this->vt)
        this->vt->draw(y, x, h, w);
    else
        drawchildren();
}

void NODE::drawchildren() const /* Draw all children of n. */
{
    this->c1->draw();
    if (this->t == HORIZONTAL)
        mvvline(this->y, this->x + this->w / 2, ACS_VLINE, this->h);
    else
        mvhline(this->y + this->h / 2, this->x, ACS_HLINE, this->w);
    wnoutrefresh(stdscr);
    this->c2->draw();
}

bool NODE::IN(int y, int x) const
{
    return (y >= this->y && y <= this->y + this->h && x >= this->x &&
            x <= this->x + this->w);
}

std::shared_ptr<NODE> NODE::findnode(int y,
                                     int x) /* Find the node enclosing y,x. */
{
    if (IN(y, x))
    {
        if (this->c1 && this->c1->IN(y, x))
            return this->c1->findnode(y, x);
        if (this->c2 && this->c2->IN(y, x))
            return this->c2->findnode(y, x);
        return shared_from_this();
    }
    return NULL;
}

void focus(const std::shared_ptr<NODE> &n) /* Focus a node. */
{
    if (!n)
    {
        return;
    }

    if (n->isView())
    {
        lastfocused = focused;
        focused = n;
    }
    else
    {
        focus(n->c1 ? n->c1 : n->c2);
    }
}

std::shared_ptr<NODE> newview(const std::shared_ptr<NODE> &p, int y, int x,
                              int h, int w) /* Open a new view. */
{
    auto n = std::make_shared<NODE>(VIEW, p, y, x, h, w);
    n->vt = std::make_unique<VTScreen>(h, w);

    auto pid = fork_setup(n->vt->vp.get(), n.get(), &n->vt->pt, h, w);
    if (pid < 0)
    {
        // error
        if (!p)
        {
            perror("forkpty");
        }
        return nullptr;
    }
    else if (pid == 0)
    {
        // child process
        return NULL;
    }

    // setup selector
    selector::set(n->vt->pt);

    return n;
}

static std::shared_ptr<NODE>
newcontainer(Node t, const std::shared_ptr<NODE> &p, int y, int x, int h, int w,
             const std::shared_ptr<NODE> &c1,
             const std::shared_ptr<NODE> &c2) /* Create a new container */
{
    auto n = std::make_shared<NODE>(t, p, y, x, h, w);
    n->c1 = c1;
    n->c2 = c2;
    c1->p = c2->p = n;
    n->reshapechildren();
    return n;
}

static void replacechild(std::shared_ptr<NODE> n,
                         const std::shared_ptr<NODE> &c1,
                         const std::shared_ptr<NODE> &c2)
{
    c2->p = n;
    if (!n)
    {
        root = c2;
        c2->reshape(0, 0, LINES, COLS);
    }
    else if (n->c1 == c1)
        n->c1 = c2;
    else if (n->c2 == c1)
        n->c2 = c2;

    if (!n)
    {
        n = root;
    }
    n->reshape(n->y, n->x, n->h, n->w);
    n->draw();
}

static void
removechild(const std::shared_ptr<NODE> &p,
            const std::shared_ptr<NODE> &c) /* Replace p with other child. */
{
    replacechild(p->p.lock(), p, c == p->c1 ? p->c2 : p->c1);
}

void deletenode(const std::shared_ptr<NODE> &n) /* Delete a node. */
{
    auto p = n->p.lock();
    if (!n || !p)
    {
        if (root)
        {
            root = nullptr;
        }
        return;
    }

    if (n == focused.lock())
    {
        focus(p->c1 == n ? p->c2 : p->c1);
    }
    removechild(p, n);
}

void split(const std::shared_ptr<NODE> &n, const Node t) /* Split a node. */
{
    int nh = t == VERTICAL ? (n->h - 1) / 2 : n->h;
    int nw = t == HORIZONTAL ? (n->w) / 2 : n->w;
    auto p = n->p.lock();
    auto v = newview(NULL, 0, 0, MAX(0, nh), MAX(0, nw));
    if (!v)
    {
        return;
    }

    auto c = newcontainer(t, n->p.lock(), n->y, n->x, n->h, n->w, n, v);
    if (!c)
    {
        return;
    }

    replacechild(p, n, c);
    focus(v);
    (p ? p : root)->draw();
}

void NODE::processVT() /* Recursively check all ptty's for input. */
{
    if (this->c1)
    {
        this->c1->processVT();
    }

    if (this->c2)
    {
        this->c2->processVT();
    }

    if (this->vt)
    {
        if (!vt->process())
        {
            deletenode(shared_from_this());
        }
    }
}
