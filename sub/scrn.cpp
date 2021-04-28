#include "node.h"
#include "scrn.h"
#include "minmax.h"
#include "pair.h"
#include <bits/stdint-uintn.h>
#include <curses.h>
#include <pstl/pstl_config.h>

struct SCRNImpl
{
    WINDOW *win = nullptr;
    short sp = 0;
    attr_t sattr = {};

    SCRNImpl(int lines, int cols)
    {
        this->win = newpad(lines, cols);
        nodelay(this->win, TRUE);
        scrollok(this->win, TRUE);
        keypad(this->win, TRUE);
    }
    ~SCRNImpl()
    {
        delwin(this->win);
    }

    void resize(int lines, int cols)
    {
        wresize(this->win, lines, cols);
    }

    std::tuple<int, int> cursor() const
    {
        int y, x;
        getyx(this->win, y, x);
        return {y, x};
    }

    bool cursor(int y, int x)
    {
        return wmove(this->win, y, x) == OK;
    }

    void getAttr()
    {
        /* save attrs and color pair  */
        wattr_get(win, &this->sattr, &this->sp, NULL);
    }

    void setAttr()
    {
        /* get attrs and color pair  */
        wattr_set(win, this->sattr, this->sp, NULL);
    }

    void setColor(short fg, short bg)
    {
        int cp = mtm_alloc_pair(fg, bg);
        wcolor_set(this->win, cp, NULL);
        cchar_t c;
        setcchar(&c, L" ", A_NORMAL, cp, NULL);
        wbkgrndset(this->win, &c);
    }

    void scr(int y)
    {
        wscrl(win, y);
    }

    bool scrollRegion(int top, int bottom)
    {
        return wsetscrreg(this->win, top, bottom) == OK;
    }

    std::tuple<int, int> scrollRegion() const
    {
        int top = 0, bot = 0;
        wgetscrreg(win, &top, &bot);
        return {top, bot};
    }

    void refresh(int pminrow, int pmincol, int sminrow, int smincol,
                 int smaxrow, int smaxcol)
    {
        pnoutrefresh(this->win, pminrow, pmincol, sminrow, smincol, smaxrow,
                     smaxcol);
    }

    CursesInput input() const
    {
        wint_t k = 0;
        int r = wget_wch(this->win, &k);
        return {r, k};
    }

    std::tuple<int, int, int, int, int, int, int, int, int>
    output(int tos) const
    {
        // COMMONVARS     - All of the common variables for a handler.
        //                  x, y     - cursor position
        //                  mx, my   - max possible values for x and y
        //                  px, py   - physical cursor position in scrollback
        //                  n        - the current node
        //                  win      - the current window
        //                  top, bot - the scrolling region
        //                  tos      - top of the screen in the pad
        //                  s        - the current SCRN buffer

        int py, px, y, x, my, mx, top = 0, bot = 0;
        getyx(win, py, px);
        y = py - tos;
        x = px;
        getmaxyx(win, my, mx);
        my -= tos;
        wgetscrreg(win, &top, &bot);
        bot++;
        bot -= tos;
        top = top <= tos ? 0 : top - tos;

        return {py, px, y, x, my, mx, top, bot, tos};
    }

    void del()
    {
        wdelch(win);
    }

    void insert(const wchar_t *ch, int n)
    {
        wins_nwstr(win, ch, n);
    }

    void add(const chtype *ch, int n)
    {
        waddchnstr(win, ch, n);
    }

    void add(const cchar_t *ch, int n)
    {
        wadd_wchnstr(win, ch, n);
    }

    void add(const wchar_t *ch, int n)
    {
        waddnwstr(win, ch, n);
    }

    void add(const char *msg)
    {
        waddstr(win, msg);
    }

    void clear_to_eol()
    {
        wclrtoeol(win);
    }

    void clear_to_bottom()
    {
        wclrtobot(win);
    }

    void erase()
    {
        werase(win);
    }

    void attr(int attr)
    {
        wattrset(win, attr);
    }

    void color(short color, void *opts)
    {
        wcolor_set(win, color, opts);
    }

    void bg(chtype ch)
    {
        wbkgdset(win, ch);
    }

    void bg(const cchar_t *ch)
    {
        wbkgrndset(win, ch);
    }
};

SCRN::SCRN(int lines, int cols) : m_impl(new SCRNImpl(lines, cols))
{
}

SCRN::~SCRN()
{
    delete m_impl;
}

WINDOW *SCRN::win() const
{
    return m_impl->win;
}

void SCRN::scrollbottom()
{
    this->off = this->tos;
}

void SCRN::fixcursor(
    int h, bool focus) /* Move the terminal cursor to the active view. */
{
    if (!focus)
    {
        return;
    }
    curs_set(this->off == this->tos ? this->vis : 0);

    int y, x;
    std::tie(y, x) = m_impl->cursor();

    m_impl->cursor(MIN(MAX(y, this->tos), this->tos + h - 1), x);
}

void SCRN::scrollback(int h)
{
    this->off = MAX(0, this->off - h / 2);
}

void SCRN::scrollforward(int h)
{
    this->off = MIN(this->tos, this->off + h / 2);
}

void SCRN::getAttr()
{
    m_impl->getAttr();
}

void SCRN::setAttr()
{
    m_impl->setAttr();
}

void SCRN::save()
{
    int y, x;
    std::tie(y, x) = m_impl->cursor();
    this->sx = x; /* save X position            */
    this->sy = y; /* save Y position            */
    this->getAttr();
    this->sfg = this->fg;     /* save foreground color      */
    this->sbg = this->bg;     /* save background color      */
    this->oxenl = this->xenl; /* save xenl state            */
    this->saved = true;       /* save data is valid         */
}

bool SCRN::restore()
{
    if (!this->saved)
        return false;
    m_impl->cursor(this->sy, this->sx); /* get old position          */
    this->setAttr();
    this->fg = this->sfg;     /* get foreground color      */
    this->bg = this->sbg;     /* get background color      */
    this->xenl = this->oxenl; /* get xenl state            */

    /* restore colors */
    m_impl->setColor(this->fg, this->bg);

    return true;
}

void SCRN::reset()
{
    auto s = this;
    s->insert = s->oxenl = s->xenl = false;
}

void SCRN::scr(int y)
{
    m_impl->scr(y);
}

bool SCRN::scrollRegion(int top, int bottom)
{
    return m_impl->scrollRegion(top, bottom);
}

std::tuple<int, int> SCRN::scrollRegion() const
{
    return m_impl->scrollRegion();
}

void SCRN::refresh(int pminrow, int pmincol, int sminrow, int smincol,
                   int smaxrow, int smaxcol)
{
    m_impl->refresh(pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);
}

CursesInput SCRN::input() const
{
    return m_impl->input();
}

std::tuple<int, int, int, int, int, int, int, int, int> SCRN::output() const
{
    return m_impl->output(this->tos);
}

std::tuple<int, int> SCRN::cursor() const
{
    return m_impl->cursor();
}

bool SCRN::cursor(int y, int x)
{
    return m_impl->cursor(y, x);
}

void SCRN::del()
{
    m_impl->del();
}

void SCRN::ins(const wchar_t *ch, int n)
{
    m_impl->insert(ch, n);
}

void SCRN::add(const chtype *ch, int n)
{
    m_impl->add(ch, n);
}

void SCRN::add(const cchar_t *ch, int n)
{
    m_impl->add(ch, n);
}

void SCRN::add(const wchar_t *ch, int n)
{
    m_impl->add(ch, n);
}

void SCRN::add(const char *p)
{
    m_impl->add(p);
}

void SCRN::clear_to_eol()
{
    m_impl->clear_to_eol();
}

void SCRN::clear_to_bottom()
{
    m_impl->clear_to_bottom();
}

void SCRN::erase()
{
    m_impl->erase();
}

void SCRN::attr(int attr)
{
    m_impl->attr(attr);
}

void SCRN::color(short color, void *opts)
{
    m_impl->color(color, opts);
}

void SCRN::bkg(chtype ch)
{
    m_impl->bg(ch);
}

void SCRN::bkg(const cchar_t *ch)
{
    m_impl->bg(ch);
}

void SCRN::resize(int lines, int cols)
{
    m_impl->resize(lines, cols);
}
