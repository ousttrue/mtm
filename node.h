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

struct SCRN
{
    int sy, sx, vis, tos, off;
    short fg, bg, sfg, sbg, sp;
    bool insert, oxenl, xenl, saved;
    attr_t sattr;
    WINDOW *win;
};

struct NODE
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
    int pt = 0;
    int ntabs = 0;
    std::vector<bool> tabs;
    bool pnm = false;
    bool decom = false;
    bool am = false;
    bool lnm = false;
    wchar_t repc;
    NODE *p = nullptr;
    NODE *c1 = nullptr;
    NODE *c2 = nullptr;
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

public:
    NODE(Node t, NODE *p, int y, int x, int h, int w);
    ~NODE();
    void reshape(int y, int x, int h, int w);
    void reshapechildren();
    void reshapeview(int d, int ow);
    void draw() const;
    void drawchildren() const;
    void detachchildren()
    {
        this->c1 = nullptr;
        this->c2 = nullptr;
    }
};

extern NODE *root;
extern NODE *focused;
extern NODE *lastfocused;
