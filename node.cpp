#include "node.h"
#include "app.h"
#include "minmax.h"
#include "selector.h"
#include "curses_term.h"
#include <curses.h>

std::tuple<Rect, Rect> Splitter::split(const Rect &rect) const
{
    if (isHorizontal)
    {
        return rect.splitHorizontal();
    }
    else
    {
        return rect.splitVertical();
    }
}

void Splitter::drawSeparator(const Rect &rect) const
{
    if (isHorizontal)
    {
        mvvline(rect.y, rect.x + rect.w / 2, ACS_VLINE, rect.h);
    }
    else
    {
        mvhline(rect.y + rect.h / 2, rect.x, ACS_HLINE, rect.w);
    }
}

Rect Splitter::viewRect(const Rect &rect) const
{
    int nh = !isHorizontal ? (rect.h - 1) / 2 : rect.h;
    int nw = isHorizontal ? (rect.w) / 2 : rect.w;
    return Rect(0, 0, MAX(0, nh), MAX(0, nw));
}

NODE::NODE(const Rect &rect) : m_rect(rect)
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
    Rect r1, r2;
    std::tie(r1, r2) = this->splitter.split(m_rect);
    this->m_child1->reshape(r1);
    this->m_child2->reshape(r2);
}

void NODE::draw() const /* Draw a node. */
{
    if (this->term)
    {
        this->term->draw(m_rect);
    }
    else
    {
        this->m_child1->draw();
        this->splitter.drawSeparator(m_rect);
        wnoutrefresh(stdscr);
        this->m_child2->draw();
    }
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

void NODE::replacechild(const std::shared_ptr<NODE> &c1,
                        const std::shared_ptr<NODE> &c2)
{
    if (this->m_child1 == c1)
    {
        this->child1(c2);
    }
    else if (this->m_child2 == c1)
    {
        this->child2(c2);
    }
    else
    {
        throw std::exception();
    }
    this->reshape(this->m_rect);
    this->draw();
}

//
// c
// nv
//
void NODE::split(bool isHorizontal) /* Split a node. */
{
    auto rect = this->splitter.viewRect(this->m_rect);
    auto v = std::make_shared<NODE>(rect);
    v->term = new_term(rect);

    // split
    auto p = this->parent();

    auto c = std::make_shared<NODE>(this->m_rect);
    c->child1(shared_from_this());
    c->child2(v);
    c->splitter.isHorizontal = isHorizontal;
    c->reshapechildren();

    if (p)
    {
        p->replacechild(shared_from_this(), c);
    }
    else
    {
        global::root(c);
    }
    global::focus(v);
    (p ? p : global::root())->draw();
}

void NODE::process()
{
    processVT();
    deleteClosed();
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
            this->closed = true;
        }
    }
}

void NODE::deletenode(const std::shared_ptr<NODE> &n) /* Delete a node. */
{
    auto other = n == this->m_child1 ? this->m_child2 : this->m_child1;
    if (n == global::focus())
    {
        global::focus(other);
    }

    auto parent = this->parent();
    if (parent)
    {
        parent->replacechild(shared_from_this(), other);
    }
    else
    {
        global::root(other);
    }
}

void NODE::deleteClosed()
{
    if (!this->term)
    {
        if (this->m_child1->closed)
        {
            deletenode(this->m_child1);
        }
        else if (this->m_child2->closed)
        {
            deletenode(this->m_child2);
        }
        else
        {
            this->m_child1->deleteClosed();
            this->m_child2->deleteClosed();
        }
    }
}

std::shared_ptr<NODE> NODE::findViewNode()
{
    if (this->term)
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
