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

class NODE : public std::enable_shared_from_this<NODE>
{
    Node m_type;
    std::weak_ptr<NODE> m_parent;
    std::shared_ptr<NODE> m_child1;
    std::shared_ptr<NODE> m_child2;

public:
    Rect m_rect;
    std::unique_ptr<CursesTerm> term;
    NODE(Node type, const std::shared_ptr<NODE> &parent, const Rect &rect);
    ~NODE();
    bool isView() const
    {
        return m_type == VIEW;
    }
    void reshape(const Rect &rect);
    void reshapechildren();
    void draw() const;
    void drawchildren() const;
    std::shared_ptr<NODE> findnode(const YX &yx);
    void processVT();

    std::shared_ptr<NODE> child1() const
    {
        return m_child1;
    }
    void child1(const std::shared_ptr<NODE> &node)
    {
        m_child1 = node;
    }
    std::shared_ptr<NODE> child2() const
    {
        return m_child2;
    }
    void child2(const std::shared_ptr<NODE> &node)
    {
        m_child2 = node;
    }
    std::shared_ptr<NODE> parent() const
    {
        return m_parent.lock();
    }
    void parent(const std::shared_ptr<NODE> &node)
    {
        m_parent = node;
    }
    std::shared_ptr<NODE> findViewNode();
};

void focus(const std::shared_ptr<NODE> &n);
void deletenode(const std::shared_ptr<NODE> &n);
void split(const std::shared_ptr<NODE> &n, const Node t);
