#pragma once

struct NODE
{
    Node t;
    int y, x, h, w, pt, ntabs;
    bool *tabs, pnm, decom, am, lnm;
    wchar_t repc;
    NODE *p, *c1, *c2;
    SCRN pri, alt, *s;
    wchar_t *g0, *g1, *g2, *g3, *gc, *gs, *sgc, *sgs;
    VTPARSER vp;

    void reshape(int y, int x, int h, int w);
    void reshapechildren();
    void reshapeview(int d, int ow);
    void draw() const;
    void drawchildren() const;
};
