#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <list>
#include "rect.h"

class Content;

struct Splitter
{
    bool isHorizontal;
    std::list<std::shared_ptr<class NODE>> chidlren;
    std::tuple<Rect, Rect> split(const Rect &rect) const;
    void drawSeparator(const Rect &rect) const;
    Rect viewRect(const Rect &rect) const;
};

class NODE : public std::enable_shared_from_this<NODE>
{
    bool m_closed = false;
    Rect m_rect = {};
    Splitter m_splitter = {};
    std::unique_ptr<Content> m_term;

public:
    NODE();
    NODE(const Rect &rect, Content *term = nullptr);
    NODE(Content *term);
    ~NODE();
    Rect rect() const
    {
        return m_rect;
    }
    Content *term() const
    {
        return m_term.get();
    }
    void term(Content *c);
    void term(std::unique_ptr<Content> c);
    void close()
    {
        m_closed = true;
    }
    void draw(const std::shared_ptr<NODE> &focus) const;
    bool process();
    std::shared_ptr<NODE> findnode(const YX &yx);
    std::tuple<std::shared_ptr<NODE>, std::shared_ptr<NODE>>
    split(bool isHorizontal);
    std::tuple<std::shared_ptr<NODE>, std::shared_ptr<NODE>> splitHorizontal()
    {
        return split(true);
    }
    std::tuple<std::shared_ptr<NODE>, std::shared_ptr<NODE>> splitVertical()
    {
        return split(false);
    }
    std::shared_ptr<NODE> findViewNode();

private:
    void reshape(const Rect &rect);
    void moveFrom(const std::shared_ptr<NODE> &from);
    void processVT();
    void deleteClosed();
};
