#pragma once
#include <memory>
#include <vector>
#include "rect.h"

enum Node
{
    HORIZONTAL,
    VERTICAL,
    VIEW
};

struct CursesTerm;

struct NODE : std::enable_shared_from_this<NODE>
{
private:
    Node t;

public:
    bool isView() const
    {
        return t == VIEW;
    }

    Rect m_rect;

    // node tree
    std::weak_ptr<NODE> p;
    std::shared_ptr<NODE> c1;
    std::shared_ptr<NODE> c2;

    std::unique_ptr<CursesTerm> term;

public:
    NODE(Node t, const std::shared_ptr<NODE> &p, const Rect &rect);
    ~NODE();
    void reshape(const Rect &rect);
    void reshapechildren();
    void draw() const;
    void drawchildren() const;
    std::shared_ptr<NODE> findnode(const YX &yx);
    void processVT();

    std::shared_ptr<NODE> findViewNode();
};

void focus(const std::shared_ptr<NODE> &n);
void deletenode(const std::shared_ptr<NODE> &n);
std::shared_ptr<NODE> newview(const Rect &rect);
void split(const std::shared_ptr<NODE> &n, const Node t);
