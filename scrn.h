#pragma once
#include <curses.h>

struct SCRN
{
    int sy;
    int sx;
    int vis;
    int tos;
    int off;
    short fg;
    short bg;
    short sfg;
    short sbg;
    bool insert;
    bool oxenl;
    bool xenl;
    bool saved;

private:
    attr_t sattr;
    short sp;

public:
    WINDOW *win;

public:
    void scrollbottom();
    void fixcursor(int h);
    void scrollback(int h);
    void scrollforward(int h);
    void getAttr();
    void setAttr();
};
