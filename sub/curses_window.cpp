#include "node.h"
#include "curses_window.h"
#include "minmax.h"
#include "pair.h"
#include <bits/stdint-uintn.h>
#include <curses.h>
#include <pstl/pstl_config.h>

struct SCRNImpl
{
    WINDOW *win = nullptr;
    // saved color pair
    short sp = 0;
    // saved attribute
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

CursesWindow::CursesWindow(int lines, int cols) : m_impl(new SCRNImpl(lines, cols))
{
}

CursesWindow::~CursesWindow()
{
    delete m_impl;
}

WINDOW *CursesWindow::win() const
{
    return m_impl->win;
}

void CursesWindow::scrollbottom()
{
    this->off = this->tos;
}

void CursesWindow::fixcursor(
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

void CursesWindow::scrollback(int h)
{
    this->off = MAX(0, this->off - h / 2);
}

void CursesWindow::scrollforward(int h)
{
    this->off = MIN(this->tos, this->off + h / 2);
}

void CursesWindow::getAttr()
{
    m_impl->getAttr();
}

void CursesWindow::setAttr()
{
    m_impl->setAttr();
}

void CursesWindow::save()
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

bool CursesWindow::restore()
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

void CursesWindow::reset()
{
    auto s = this;
    s->insert = s->oxenl = s->xenl = false;
}

void CursesWindow::scr(int y)
{
    m_impl->scr(y);
}

bool CursesWindow::scrollRegion(int top, int bottom)
{
    return m_impl->scrollRegion(top, bottom);
}

std::tuple<int, int> CursesWindow::scrollRegion() const
{
    return m_impl->scrollRegion();
}

void CursesWindow::refresh(int pminrow, int pmincol, int sminrow, int smincol,
                   int smaxrow, int smaxcol)
{
    m_impl->refresh(pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);
}

CursesInput CursesWindow::input() const
{
    return m_impl->input();
}

std::tuple<int, int, int, int, int, int, int, int, int> CursesWindow::output() const
{
    return m_impl->output(this->tos);
}

std::tuple<int, int> CursesWindow::cursor() const
{
    return m_impl->cursor();
}

bool CursesWindow::cursor(int y, int x)
{
    return m_impl->cursor(y, x);
}

void CursesWindow::del()
{
    m_impl->del();
}

void CursesWindow::ins(const wchar_t *ch, int n)
{
    m_impl->insert(ch, n);
}

void CursesWindow::add(const chtype *ch, int n)
{
    m_impl->add(ch, n);
}

void CursesWindow::add(const cchar_t *ch, int n)
{
    m_impl->add(ch, n);
}

void CursesWindow::add(const wchar_t *ch, int n)
{
    m_impl->add(ch, n);
}

void CursesWindow::add(const char *p)
{
    m_impl->add(p);
}

void CursesWindow::clear_to_eol()
{
    m_impl->clear_to_eol();
}

void CursesWindow::clear_to_bottom()
{
    m_impl->clear_to_bottom();
}

void CursesWindow::erase()
{
    m_impl->erase();
}

void CursesWindow::attr(int attr)
{
    m_impl->attr(attr);
}

void CursesWindow::color(short color, void *opts)
{
    m_impl->color(color, opts);
}

void CursesWindow::bkg(chtype ch)
{
    m_impl->bg(ch);
}

void CursesWindow::bkg(const cchar_t *ch)
{
    m_impl->bg(ch);
}

void CursesWindow::resize(int lines, int cols)
{
    m_impl->resize(lines, cols);
}
