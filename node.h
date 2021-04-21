#pragma once
#include <functional>
#include <memory>
#include <vector>
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
    std::weak_ptr<NODE> m_parent;
    std::shared_ptr<NODE> m_child1;
    std::shared_ptr<NODE> m_child2;

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
    void reshape(const Rect &rect);
    std::shared_ptr<NODE> findViewNode();
    void split(bool isHorizontal);

private:
    std::shared_ptr<NODE> parent() const
    {
        return m_parent.lock();
    }
    void reshapechildren();
    void drawchildren() const;
    void processVT();
    void deleteClosed();
    void deletenode(const std::shared_ptr<NODE> &n);

    void child1(const std::shared_ptr<NODE> &node)
    {
        if (this == node.get())
        {
            throw std::exception();
        }
        m_child1 = node;
        node->m_parent = shared_from_this();
    }
    void child2(const std::shared_ptr<NODE> &node)
    {
        if (this == node.get())
        {
            throw std::exception();
        }
        m_child2 = node;
        node->m_parent = shared_from_this();
    }
    void replacechild(const std::shared_ptr<NODE> &c1,
                      const std::shared_ptr<NODE> &c2);

};
