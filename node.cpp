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

void NODE::moveFrom(const std::shared_ptr<NODE> &from)
{
    if (from->term)
    {
        this->term = std::move(from->term);
    }
    else
    {
        this->m_chidlren = from->m_chidlren;
        from->m_chidlren.clear();
    }
}

void NODE::reshape(const Rect &rect) /* Reshape a node. */
{
    int d = m_rect.h - rect.h;
    int ow = m_rect.w;
    m_rect = {
        rect.y,
        rect.x,
        MAX(rect.h, 1),
        MAX(rect.w, 1),
    };

    if (this->term)
    {
        this->term->reshapeview(d, ow, m_rect);
    }
    else
    {
        if (m_chidlren.size() != 2)
        {
            throw std::exception();
        }
        Rect r1, r2;
        std::tie(r1, r2) = this->splitter.split(m_rect);

        auto child1 = m_chidlren.front();
        auto child2 = m_chidlren.back();
        child1->reshape(r1);
        child2->reshape(r2);
    }
}

void NODE::draw() const /* Draw a node. */
{
    if (this->term)
    {
        this->term->draw(m_rect);
    }
    else
    {
        auto it = m_chidlren.begin();
        (*it)->draw();
        this->splitter.drawSeparator(m_rect);
        wnoutrefresh(stdscr);
        ++it;
        (*it)->draw();
    }
}

std::shared_ptr<NODE>
NODE::findnode(const YX &p) /* Find the node enclosing y,x. */
{
    if (m_rect.contains(p))
    {
        for (auto &child : m_chidlren)
        {
            if (auto found = child->findnode(p))
            {
                return found;
            }
        }
        return shared_from_this();
    }
    return NULL;
}

//
// c
// nv
//
void NODE::split(bool isHorizontal) /* Split a node. */
{
    auto rect = this->splitter.viewRect(this->m_rect);
    auto v = std::make_shared<NODE>(rect);
    v->term.reset(CursesTerm::create(rect));

    auto c = std::make_shared<NODE>(this->m_rect);
    c->moveFrom(shared_from_this());
    this->m_chidlren.push_back(c);
    this->m_chidlren.push_back(v);
    this->splitter.isHorizontal = isHorizontal;

    global::focus(v);
}

void NODE::process()
{
    processVT();
    if (!term)
    {
        deleteClosed();
    }
}

void NODE::processVT() /* Recursively check all ptty's for input. */
{
    for (auto &child : m_chidlren)
    {
        child->processVT();
    }

    if (this->term)
    {
        if (!term->process())
        {
            this->closed = true;
        }
    }
}

void NODE::deleteClosed()
{
    if (this->term)
    {
        // throw std::exception();
        return;
    }

    auto child1 = m_chidlren.front();
    auto child2 = m_chidlren.back();
    auto focusThis = false;
    if (child1->closed)
    {
        focusThis = global::focus() == child1;
        this->moveFrom(child2);
        m_chidlren.clear();
    }
    else if (child2->closed)
    {
        focusThis = global::focus() == child2;
        this->moveFrom(child1);
        m_chidlren.clear();
    }
    else
    {
        child1->deleteClosed();
        child2->deleteClosed();
    }
    if (focusThis)
    {
        global::focus(shared_from_this());
    }
}

std::shared_ptr<NODE> NODE::findViewNode()
{
    if (this->term)
    {
        return shared_from_this();
    }

    for (auto &child : m_chidlren)
    {
        if (auto found = child->findViewNode())
        {
            return found;
        }
    }

    return nullptr;
}
