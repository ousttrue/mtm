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
    std::tuple<Rect, Rect> split(const Rect &rect) const;
    void drawSeparator(const Rect &rect) const;
    Rect viewRect(const Rect &rect) const;
};

class NODE : public std::enable_shared_from_this<NODE>
{
    std::list<std::shared_ptr<NODE>> m_chidlren;

public:
    bool closed = false;
    Rect m_rect;
    Splitter splitter;
    std::unique_ptr<CursesTerm> term;

    NODE(const Rect &rect);
    ~NODE();

    void draw() const;
    void process();
    std::shared_ptr<NODE> findnode(const YX &yx);
    std::shared_ptr<NODE> findViewNode();
    void split(bool isHorizontal);
    void reshape(const Rect &rect);

private:
    void moveFrom(const std::shared_ptr<NODE> &from);
    void processVT();
    void deleteClosed();
};
