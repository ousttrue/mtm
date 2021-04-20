#include "node.h"
#include "app.h"
#include "minmax.h"
#include "selector.h"
#include "curses_term.h"
#include <curses.h>

NODE::NODE(Node t, const Rect &rect) : m_type(t), m_rect(rect)
{
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
    if (this->m_type == HORIZONTAL)
    {
        Rect left, right;
        std::tie(left, right) = m_rect.splitHorizontal();
        this->m_child1->reshape(left);
        this->m_child2->reshape(right);
    }
    else if (this->m_type == VERTICAL)
    {
        Rect top, bottom;
        std::tie(top, bottom) = m_rect.splitVertical();
        this->m_child1->reshape(top);
        this->m_child1->reshape(bottom);
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
    this->m_child1->draw();
    if (this->m_type == HORIZONTAL)
        mvvline(m_rect.y, m_rect.x + m_rect.w / 2, ACS_VLINE, m_rect.h);
    else
        mvhline(m_rect.y + m_rect.h / 2, m_rect.x, ACS_HLINE, m_rect.w);
    wnoutrefresh(stdscr);
    this->m_child2->draw();
}

std::shared_ptr<NODE>
NODE::findnode(const YX &p) /* Find the node enclosing y,x. */
{
    if (m_rect.contains(p))
    {
        if (this->m_child1 && this->m_child1->m_rect.contains(p))
            return this->m_child1->findnode(p);
        if (this->m_child2 && this->m_child2->m_rect.contains(p))
            return this->m_child2->findnode(p);
        return shared_from_this();
    }
    return NULL;
}

static void replacechild(std::shared_ptr<NODE> n,
                         const std::shared_ptr<NODE> &c1,
                         const std::shared_ptr<NODE> &c2)
{
    if (n->child1() == c1)
    {
        n->child1(c2);
    }
    else if (n->child2() == c1)
    {
        n->child2(c2);
    }
    else
    {
        throw std::exception();
    }
    n->reshape(n->m_rect);
    n->draw();
}

static void
removechild(const std::shared_ptr<NODE> &p,
            const std::shared_ptr<NODE> &c) /* Replace p with other child. */
{
    auto parent = p->parent();
    auto other = c == p->child1() ? p->child2() : p->child1();
    if (parent)
    {
        replacechild(parent, p, other);
    }
    else
    {
        global::root(other);
    }
}

void deletenode(const std::shared_ptr<NODE> &n) /* Delete a node. */
{
    auto p = n->parent();
    if (!p)
    {
        global::quit();
        return;
    }

    if (n == global::focus())
    {
        global::focus(p->child1() == n ? p->child2() : p->child1());
    }

    removechild(p, n);
}

static std::shared_ptr<NODE>
newcontainer(Node t, const Rect &rect, const std::shared_ptr<NODE> &c1,
             const std::shared_ptr<NODE> &c2) /* Create a new container */
{
    auto n = std::make_shared<NODE>(t, rect);
    n->child1(c1);
    n->child2(c2);
    n->reshapechildren();
    return n;
}

//
// c
// nv
//
void split(const std::shared_ptr<NODE> &n, const Node t) /* Split a node. */
{
    // new view
    int nh = t == VERTICAL ? (n->m_rect.h - 1) / 2 : n->m_rect.h;
    int nw = t == HORIZONTAL ? (n->m_rect.w) / 2 : n->m_rect.w;
    auto rect = Rect(0, 0, MAX(0, nh), MAX(0, nw));
    auto v = std::make_shared<NODE>(VIEW, rect);
    v->term = new_term(rect);

    // split
    auto p = n->parent();
    auto c = newcontainer(t, n->m_rect, n, v);
    if (p)
    {
        replacechild(p, n, c);
    }
    else
    {
        global::root(c);
        // c->reshape(c->m_rect);
        // c->draw();
    }
    global::focus(v);
    (p ? p : global::root())->draw();
}

void NODE::processVT() /* Recursively check all ptty's for input. */
{
    if (this->m_child1)
    {
        this->m_child1->processVT();
    }

    if (this->m_child2)
    {
        this->m_child2->processVT();
    }

    if (this->term)
    {
        if (!term->process())
        {
            deletenode(shared_from_this());
        }
    }
}

std::shared_ptr<NODE> NODE::findViewNode()
{
    if (isView())
    {
        return shared_from_this();
    }

    if (m_child1)
    {
        if (auto found = m_child1->findViewNode())
        {
            return found;
        }
    }

    if (m_child2)
    {
        if (auto found = m_child2->findViewNode())
        {
            return found;
        }
    }

    return nullptr;
}
