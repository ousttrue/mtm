#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <list>
#include "rect.h"

struct CursesTerm;

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

public:
    std::unique_ptr<CursesTerm> term;

    NODE(const Rect &rect);
    ~NODE();
    void close()
    {
        m_closed = true;
    }
    void draw() const;
    bool process();
    std::shared_ptr<NODE> findnode(const YX &yx);
    void split(bool isHorizontal);
    std::shared_ptr<NODE> findViewNode();

private:
    void reshape(const Rect &rect);
    void moveFrom(const std::shared_ptr<NODE> &from);
    void processVT();
    void deleteClosed();
};
