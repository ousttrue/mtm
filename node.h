#pragma once

#include "vt/vtparser.h"
#include <curses.h>
#include <memory>
#include <vector>

enum Node
{
    HORIZONTAL,
    VERTICAL,
    VIEW
};

struct SCRN;

struct VT
{
    int pt = -1;
    int ntabs = 0;
    std::vector<bool> tabs;
    bool pnm = false;
    bool decom = false;
    bool am = false;
    bool lnm = false;
    wchar_t repc;
    std::shared_ptr<SCRN> pri;
    std::shared_ptr<SCRN> alt;
    std::shared_ptr<SCRN> s;
    wchar_t *g0 = nullptr;
    wchar_t *g1 = nullptr;
    wchar_t *g2 = nullptr;
    wchar_t *g3 = nullptr;
    wchar_t *gc = nullptr;
    wchar_t *gs = nullptr;
    wchar_t *sgc = nullptr;
    wchar_t *sgs = nullptr;
    VTPARSER vp = {};

    VT(int lines, int cols);
    ~VT();
    void reshapeview(int d, int ow, int lines, int cols);
    void draw(int y, int x, int h, int w);
    bool process();
    bool handleUserInput();
    void fixCursor(int h);
    void reset(int h);
    bool alternate_screen_buffer_mode(bool set);
};

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

    std::unique_ptr<VT> vt;

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
