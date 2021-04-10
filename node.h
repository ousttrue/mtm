#pragma once

#include "vt/vtparser.h"
#include <curses.h>
#include <memory>

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
    Node t;
    int y, x, h, w, pt, ntabs;
    bool *tabs, pnm, decom, am, lnm;
    wchar_t repc;
    NODE *p, *c1, *c2;
    std::shared_ptr<SCRN> pri;
    std::shared_ptr<SCRN> alt;
    std::shared_ptr<SCRN> s;
    wchar_t *g0, *g1, *g2, *g3, *gc, *gs, *sgc, *sgs;
    VTPARSER vp;

    void reshape(int y, int x, int h, int w);
    void reshapechildren();
    void reshapeview(int d, int ow);
    void draw() const;
    void drawchildren() const;

    static NODE *newnode(Node t, NODE *p, int y, int x, int h, int w);
};
