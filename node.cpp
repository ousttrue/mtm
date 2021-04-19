#include "node.h"
#include "app.h"
#include "minmax.h"
#include "selector.h"
#include "curses_term.h"
#include "vthandler.h"
#include <curses.h>


NODE::NODE(Node t, const std::shared_ptr<NODE> &p, const Rect &rect)
    : m_rect(rect)
{
    this->t = t;
    this->p = p;
}

NODE::~NODE()
{
}

void NODE::reshape(const Rect &rect) /* Reshape a node. */
{
    if (m_rect == rect && this->term)
        return;

    int d = m_rect.h - rect.h;
    int ow = m_rect.w;
    m_rect = {
        rect.y,
        rect.x,
        MAX(rect.h, 1),
        MAX(rect.w, 1),
    };

    if (this->term)
        this->term->reshapeview(d, ow, m_rect);
    else
        reshapechildren();
    this->draw();
}

void NODE::reshapechildren() /* Reshape all children of a view. */
{
    if (this->t == HORIZONTAL)
    {
        Rect left, right;
        std::tie(left, right) = m_rect.splitHorizontal();
        this->c1->reshape(left);
        this->c2->reshape(right);
    }
    else if (this->t == VERTICAL)
    {
        Rect top, bottom;
        std::tie(top, bottom) = m_rect.splitVertical();
        this->c1->reshape(top);
        this->c1->reshape(bottom);
    }
}

void NODE::draw() const /* Draw a node. */
{
    if (this->term)
        this->term->draw(m_rect);
    else
        drawchildren();
}

void NODE::drawchildren() const /* Draw all children of n. */
{
    this->c1->draw();
    if (this->t == HORIZONTAL)
        mvvline(m_rect.y, m_rect.x + m_rect.w / 2, ACS_VLINE, m_rect.h);
    else
        mvhline(m_rect.y + m_rect.h / 2, m_rect.x, ACS_HLINE, m_rect.w);
    wnoutrefresh(stdscr);
    this->c2->draw();
}

std::shared_ptr<NODE> NODE::findnode(const YX &p) /* Find the node enclosing y,x. */
{
    if (m_rect.contains(p))
    {
        if (this->c1 && this->c1->m_rect.contains(p))
            return this->c1->findnode(p);
        if (this->c2 && this->c2->m_rect.contains(p))
            return this->c2->findnode(p);
        return shared_from_this();
    }
    return NULL;
}


std::shared_ptr<NODE> newview(const Rect &rect) /* Open a new view. */
{
    auto n = std::make_shared<NODE>(VIEW, nullptr, rect);
    n->term = std::make_unique<CursesTerm>(rect);

    vp_initialize(n->term);
    auto pid = fork_setup(&n->term->pt, rect);
    if (pid < 0)
    {
        // error
        perror("forkpty");
        return nullptr;
    }
    else if (pid == 0)
    {
        // child process
        return NULL;
    }

    // setup selector
    selector::set(n->term->pt);

    return n;
}

static void replacechild(std::shared_ptr<NODE> n,
                         const std::shared_ptr<NODE> &c1,
                         const std::shared_ptr<NODE> &c2)
{
    c2->p = n;
    if (!n)
    {
        global::root(c2);
        c2->reshape(Rect(0, 0, LINES, COLS));
    }
    else if (n->c1 == c1)
        n->c1 = c2;
    else if (n->c2 == c1)
        n->c2 = c2;

    if (!n)
    {
        n = global::root();
    }
    n->reshape(n->m_rect);
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
    if (!p)
    {
        global::quit();
        return;
    }

    if (n == global::focus())
    {
        global::focus(p->c1 == n ? p->c2 : p->c1);
    }
    removechild(p, n);
}

static std::shared_ptr<NODE>
newcontainer(Node t, const std::shared_ptr<NODE> &p, const Rect &rect,
             const std::shared_ptr<NODE> &c1,
             const std::shared_ptr<NODE> &c2) /* Create a new container */
{
    auto n = std::make_shared<NODE>(t, p, rect);
    n->c1 = c1;
    n->c2 = c2;
    c1->p = c2->p = n;
    n->reshapechildren();
    return n;
}

void split(const std::shared_ptr<NODE> &n, const Node t) /* Split a node. */
{
    int nh = t == VERTICAL ? (n->m_rect.h - 1) / 2 : n->m_rect.h;
    int nw = t == HORIZONTAL ? (n->m_rect.w) / 2 : n->m_rect.w;
    auto p = n->p.lock();
    auto v = newview(Rect(0, 0, MAX(0, nh), MAX(0, nw)));
    if (!v)
    {
        return;
    }

    auto c = newcontainer(t, n->p.lock(), n->m_rect, n, v);
    if (!c)
    {
        return;
    }

    replacechild(p, n, c);
    global::focus(v);
    (p ? p : global::root())->draw();
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

    if (this->term)
    {
        if (!term->process())
        {
            deletenode(shared_from_this());
        }
    }
}
