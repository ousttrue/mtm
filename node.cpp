#ifdef __cplusplus
extern "C"
{
#endif

#include "vt/vtparser.h"

#ifdef __cplusplus
}
#endif

#include "config.h"
#include "node.h"
#include "minmax.h"
#include "selector.h"
#include "scrn.h"
#include "vthandler.h"
#include <cstdlib>
#include <unistd.h>

std::shared_ptr<NODE> root;
std::weak_ptr<NODE> focused;
std::weak_ptr<NODE> lastfocused;

NODE::NODE(Node t, const std::shared_ptr<NODE> &p, int y, int x, int h, int w)
{
    this->t = t;
    this->pt = -1;
    this->p = p;
    this->y = y;
    this->x = x;
    this->h = h;
    this->w = w;
    for (int i = 0; i < w; i++)
    {
        /* keep old overlapping tabs */
        tabs.push_back(i % 8 == 0);
    }
    this->ntabs = w;
    this->pri = std::make_shared<SCRN>();
    this->alt = std::make_shared<SCRN>();
}

NODE::~NODE()
{
    if (auto l = lastfocused.lock())
        if (l.get() == this)
            lastfocused.reset();
    if (this->pri->win)
        delwin(this->pri->win);
    if (this->alt->win)
        delwin(this->alt->win);
    if (this->pt >= 0)
    {
        selector::close(this->pt);
    }
}

void NODE::reshape(int y, int x, int h, int w) /* Reshape a node. */
{
    if (this->y == y && this->x == x && this->h == h && this->w == w &&
        this->t == VIEW)
        return;

    int d = this->h - h;
    int ow = this->w;
    this->y = y;
    this->x = x;
    this->h = MAX(h, 1);
    this->w = MAX(w, 1);

    if (this->t == VIEW)
        reshapeview(d, ow);
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

void NODE::reshapeview(int d, int ow) /* Reshape a view. */
{
    int oy, ox;
    struct winsize ws = {.ws_row = (unsigned short)this->h,
                         .ws_col = (unsigned short)this->w};

    {
        this->ntabs = this->w;
        auto oldtabs = tabs;
        this->tabs.clear();
        for (int i = 0; i < w; i++) /* keep old overlapping tabs */
            tabs.push_back(i < ow ? oldtabs[i] : (i % 8 == 0));
    }

    getyx(this->s->win, oy, ox);
    wresize(this->pri->win, MAX(this->h, SCROLLBACK), MAX(this->w, 2));
    wresize(this->alt->win, MAX(this->h, 2), MAX(this->w, 2));
    this->pri->tos = this->pri->off = MAX(0, SCROLLBACK - this->h);
    this->alt->tos = this->alt->off = 0;
    wsetscrreg(this->pri->win, 0, MAX(SCROLLBACK, this->h) - 1);
    wsetscrreg(this->alt->win, 0, this->h - 1);
    if (d > 0)
    { /* make sure the new top line syncs up after reshape */
        wmove(this->s->win, oy + d, ox);
        wscrl(this->s->win, -d);
    }
    doupdate();
    refresh();
    ioctl(this->pt, TIOCSWINSZ, &ws);
}

void NODE::draw() const /* Draw a node. */
{
    if (this->t == VIEW)
        pnoutrefresh(this->s->win, this->s->off, 0, this->y, this->x,
                     this->y + this->h - 1, this->x + this->w - 1);
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
    auto n = std::make_shared<View>(VIEW, p, y, x, h, w);
    auto pri = n->pri;
    auto alt = n->alt;
    pri->win = newpad(MAX(h, SCROLLBACK), w);
    alt->win = newpad(h, w);
    if (!pri->win || !alt->win)
    {
        return nullptr;
    }

    pri->tos = pri->off = MAX(0, SCROLLBACK - h);
    n->s = pri;

    nodelay(pri->win, TRUE);
    nodelay(alt->win, TRUE);
    scrollok(pri->win, TRUE);
    scrollok(alt->win, TRUE);
    keypad(pri->win, TRUE);
    keypad(alt->win, TRUE);

    auto pid = fork_setup(&n->vp, n.get(), &n->pt, h, w);
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
    selector::set(n->pt);

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

    if (this->isView() && this->pt > 0 && selector::isSet(this->pt))
    {
        char g_iobuf[BUFSIZ];
        ssize_t r = read(this->pt, g_iobuf, sizeof(g_iobuf));
        if (r > 0)
        {
            vtwrite(&this->vp, g_iobuf, r);
        }
        if (r <= 0 && errno != EINTR && errno != EWOULDBLOCK)
        {
            deletenode(shared_from_this());
        }
    }
}

bool NODE::handleUserInput()
{
    wint_t w = 0;
    int r = wget_wch(this->s->win, &w);
    return handlechar(r, w);
}

void NODE::fixCursor()
{
    this->s->fixcursor(this->h);
}

void NODE::reset()
{
    this->gs = this->gc = this->g0 = CSET_US;
    this->g1 = CSET_GRAPH;
    this->g2 = CSET_US;
    this->g3 = CSET_GRAPH;
    this->decom = s->insert = s->oxenl = s->xenl = this->lnm = false;
    this->am = true;
    this->pnm = false;
    this->pri->vis = this->alt->vis = 1;
    this->s = this->pri;
    wsetscrreg(this->pri->win, 0, MAX(SCROLLBACK, this->h) - 1);
    wsetscrreg(this->alt->win, 0, this->h - 1);
    for (int i = 0; i < this->ntabs; i++)
        this->tabs[i] = (i % 8 == 0);
}

bool NODE::alternate_screen_buffer_mode(bool set)
{
    auto n = this;
    if (set && n->s != n->alt)
    {
        n->s = n->alt;
        return true;
        // CALL(cls);
    }
    else if (!set && n->s != n->pri)
    {
        n->s = n->pri;
    }
    return false;
}
