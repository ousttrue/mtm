#pragma once
#include <curses.h>

struct SCRN
{
    int vis = 0;
    int tos = 0;
    int off = 0;
    short fg = 0;
    short bg = 0;

public:
    bool insert = false;
    bool oxenl = false;
    bool xenl = false;

private:
    int sy = 0;
    int sx = 0;
    short sfg = 0;
    short sbg = 0;
    bool saved = false;
    attr_t sattr = {};
    short sp = 0;

public:
    WINDOW *win = nullptr;

public:
    void scrollbottom();
    void fixcursor(int h);
    void scrollback(int h);
    void scrollforward(int h);
    void getAttr();
    void setAttr();
    void save();
    bool restore();
    void reset();
};
