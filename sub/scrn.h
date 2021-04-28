#pragma once
#include <bits/types/wint_t.h>
#include <tuple>
#include <curses.h>

struct CursesInput
{
    int status = 0;
    wint_t code = 0;
};

struct SCRN
{
    struct SCRNImpl *m_impl = nullptr;

    // cursor visibility
    int vis = 0;
    // top of scroll
    int tos = 0;
    // scroll position
    int off = 0;

    short fg = 0;
    short bg = 0;

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
public:
    SCRN(int lines, int cols);
    ~SCRN();
    WINDOW* win()const;
    void scrollbottom();
    void fixcursor(int h, bool focus);
    void scrollback(int h);
    void scrollforward(int h);
    void getAttr();
    void setAttr();
    void save();
    bool restore();
    void reset();
    bool INSCR() const
    {
        return tos != off;
    }
    void scr(int y);
    bool scrollRegion(int top, int bottom);
    std::tuple<int, int> scrollRegion() const;
    void refresh(int pminrow, int pmincol, int sminrow, int smincol,
                 int smaxrow, int smaxcol);
    CursesInput input() const;
    std::tuple<int, int, int, int, int, int, int, int, int> output() const;
    bool cursor(int y, int x);
    std::tuple<int, int> cursor() const;
    void del();
    void ins(const wchar_t *ch, int n);
    void add(const chtype *ch, int n);
    void add(const cchar_t *ch, int n);
    void add(const wchar_t *ch, int n);
    void clear_to_eol();
    void clear_to_bottom();
    void erase();
    void attr(int attr);
    void color(short color, void *opts = nullptr);
    void bkg(chtype ch);
    void bkg(const cchar_t *ch);
    void resize(int lines, int cols);
};
