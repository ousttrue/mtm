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
private:
    std::weak_ptr<NODE> p;
    std::shared_ptr<NODE> c1;
    std::shared_ptr<NODE> c2;

public:
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

    std::shared_ptr<NODE> child1() const
    {
        return c1;
    }
    void child1(const std::shared_ptr<NODE> &node)
    {
        c1 = node;
    }
    std::shared_ptr<NODE> child2() const
    {
        return c2;
    }
    void child2(const std::shared_ptr<NODE> &node)
    {
        c2 = node;
    }
    std::shared_ptr<NODE> parent() const
    {
        return p.lock();
    }
    void parent(const std::shared_ptr<NODE> &node)
    {
        p = node;
    }
    std::shared_ptr<NODE> findViewNode();
};

void focus(const std::shared_ptr<NODE> &n);
void deletenode(const std::shared_ptr<NODE> &n);
std::shared_ptr<NODE> newview(const Rect &rect);
void split(const std::shared_ptr<NODE> &n, const Node t);
