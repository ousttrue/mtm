#pragma once
#include "rect.h"
#include <bits/types/wint_t.h>
#include <cstdlib>
#include <tuple>
#include <curses.h>

struct CursesInput
{
    int status = 0;
    wint_t code = 0;
};

///
/// ncurses の pad の wrapper
///
class CursesWindow
{
    WINDOW *m_win = nullptr;
    // scroll position
    int off = 0;

    // saved color pair
    short m_sp = 0;
    // saved attribute
    attr_t m_sattr = {};

public:
    // cursor visibility
    int vis = 0;
    // color
    short fg = 0;
    short bg = 0;
    // top of scroll
    int tos = 0;

public:
    // is there cursor end of line
    bool xenl = false;
    // insert mode ?
    bool insert = false;

private:
    bool oxenl = false;
    int sy = 0;
    int sx = 0;
    short sfg = 0;
    short sbg = 0;
    bool saved = false;

public:
    CursesWindow(int lines, int cols, int scrollback = 0);
    ~CursesWindow();
    WINDOW *win() const
    {
        return m_win;
    }
    void resize(int lines, int cols, int scrollback = 0);
    bool cursor(int y, int x);
    std::tuple<int, int> cursor() const;
    void getAttr();
    void setAttr();
    void setColor(short fg, short bg);

    void scrollbottom();
    void fixcursor(int h, bool focus);
    void scrollback(int h);
    void scrollforward(int h);
    void save();
    bool restore();
    void reset(int lines, int scrollback = 0);
    bool INSCR() const
    {
        return tos != off;
    }
    void scr(int y);
    bool scrollRegion(int top, int bottom);
    std::tuple<int, int> scrollRegion() const;
    void refresh(int pminrow, int pmincol, int sminrow, int smincol,
                 int smaxrow, int smaxcol);
    void refresh(const Rect &rect)
    {
        refresh(off, 0, rect.y, rect.x, rect.y + rect.h - 1,
                rect.x + rect.w - 1);
    }
    CursesInput input() const;
    std::tuple<int, int, int, int, int, int, int, int, int> output() const;
    void del();
    void ins(const wchar_t *ch, int n);
    void add(const chtype *ch, int n);
    void add(const cchar_t *ch, int n);
    void add(const wchar_t *ch, int n);
    void add(const char *msg);
    void clear_to_eol();
    void clear_to_bottom();
    void erase();
    void attr(int attr);
    void color(short color, void *opts = nullptr);
    void bkg(chtype ch);
    void bkg(const cchar_t *ch);
};
