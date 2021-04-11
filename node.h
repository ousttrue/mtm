#pragma once
#include <memory>
#include <vector>

enum Node
{
    HORIZONTAL,
    VERTICAL,
    VIEW
};

struct VTScreen;
struct NODE : std::enable_shared_from_this<NODE>
{
private:
    Node t;

public:
    bool isView() const
    {
        return t == VIEW;
    }
    int y = 0;
    int x = 0;
    int h = 0;
    int w = 0;

    // node tree
    std::weak_ptr<NODE> p;
    std::shared_ptr<NODE> c1;
    std::shared_ptr<NODE> c2;

    std::unique_ptr<VTScreen> vt;

public:
    NODE(Node t, const std::shared_ptr<NODE> &p, int y, int x, int h, int w);
    ~NODE();
    void reshape(int y, int x, int h, int w);
    void reshapechildren();
    void draw() const;
    void drawchildren() const;
    std::shared_ptr<NODE> findnode(int y, int x);
    bool IN(int y, int x) const;
    void processVT();
};

extern std::shared_ptr<NODE> root;
extern std::weak_ptr<NODE> focused;
extern std::weak_ptr<NODE> lastfocused;
void focus(const std::shared_ptr<NODE> &n);
void deletenode(const std::shared_ptr<NODE> &n);
std::shared_ptr<NODE> newview(const std::shared_ptr<NODE> &p, int y, int x,
                              int h, int w);
void split(const std::shared_ptr<NODE> &n, const Node t);
