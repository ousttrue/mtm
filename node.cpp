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

NODE::NODE(const Rect &rect, CursesTerm *term) : m_rect(rect), m_term(term)
{
}

NODE::~NODE()
{
}

void NODE::moveFrom(const std::shared_ptr<NODE> &from)
{
    if (from->m_term)
    {
        this->m_term = std::move(from->m_term);
    }
    else
    {
        this->m_splitter = from->m_splitter;
        from->m_splitter = {};
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

    if (this->m_term)
    {
        this->m_term->reshapeview(d, ow, m_rect);
    }
    else
    {
        if (m_splitter.chidlren.size() != 2)
        {
            throw std::exception();
        }
        Rect r1, r2;
        std::tie(r1, r2) = this->m_splitter.split(m_rect);

        auto child1 = m_splitter.chidlren.front();
        auto child2 = m_splitter.chidlren.back();
        child1->reshape(r1);
        child2->reshape(r2);
    }
}

void NODE::draw(const std::shared_ptr<NODE> &focus) const /* Draw a node. */
{
    if (this->m_term)
    {
        this->m_term->draw(m_rect, this == focus.get());
    }
    else
    {
        auto it = m_splitter.chidlren.begin();
        (*it)->draw(focus);
        this->m_splitter.drawSeparator(m_rect);
        wnoutrefresh(stdscr);
        ++it;
        (*it)->draw(focus);
    }
}

std::shared_ptr<NODE>
NODE::findnode(const YX &p) /* Find the node enclosing y,x. */
{
    if (m_rect.contains(p))
    {
        for (auto &child : m_splitter.chidlren)
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
    auto rect = this->m_splitter.viewRect(this->m_rect);
    auto v = std::make_shared<NODE>(rect);
    v->m_term.reset(CursesTerm::create(rect));

    auto c = std::make_shared<NODE>(this->m_rect);
    c->moveFrom(shared_from_this());
    this->m_splitter.chidlren.push_back(c);
    this->m_splitter.chidlren.push_back(v);
    this->m_splitter.isHorizontal = isHorizontal;

    App::instance().focus(v);
}

bool NODE::process()
{
    processVT();
    if (m_closed)
    {
        return false;
    }
    deleteClosed();
    reshape({0, 0, LINES, COLS});
    return true;
}

void NODE::processVT() /* Recursively check all ptty's for input. */
{
    for (auto &child : m_splitter.chidlren)
    {
        child->processVT();
    }

    if (this->m_term)
    {
        if (!m_term->process())
        {
            this->m_closed = true;
        }
    }
}

void NODE::deleteClosed()
{
    if (this->m_term)
    {
        // throw std::exception();
        return;
    }

    auto child1 = m_splitter.chidlren.front();
    auto child2 = m_splitter.chidlren.back();
    auto focusThis = false;
    if (child1->m_closed)
    {
        focusThis = App::instance().focus() == child1;
        this->moveFrom(child2);
        this->m_splitter = {};
    }
    else if (child2->m_closed)
    {
        focusThis = App::instance().focus() == child2;
        this->moveFrom(child1);
        this->m_splitter = {};
    }
    else
    {
        child1->deleteClosed();
        child2->deleteClosed();
    }
    if (focusThis)
    {
        App::instance().focus(shared_from_this());
    }
}

std::shared_ptr<NODE> NODE::findViewNode()
{
    if (this->m_term)
    {
        return shared_from_this();
    }

    for (auto &child : m_splitter.chidlren)
    {
        if (auto found = child->findViewNode())
        {
            return found;
        }
    }

    return nullptr;
}
