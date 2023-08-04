#include "node.h"
#include "curses_window.h"
#include "minmax.h"
#include "pair.h"
#include <bits/stdint-uintn.h>
#include <curses.h>
// #include <pstl/pstl_config.h>

CursesWindow::CursesWindow(int lines, int cols, int scrollback)
{
    if (scrollback > 0)
    {
        lines = MAX(lines, scrollback);
        m_maxScrollPosition = m_scrollPosition = MAX(0, scrollback - lines);
    }
    else
    {
        m_maxScrollPosition = m_scrollPosition = 0;
    }
    m_win = newpad(lines, cols);
    nodelay(m_win, TRUE);
    scrollok(m_win, TRUE);
    keypad(m_win, TRUE);
}

CursesWindow::~CursesWindow()
{
    delwin(m_win);
}

void CursesWindow::resize(int lines, int cols, int scrollback)
{
    if (scrollback > 0)
    {
        lines = MAX(lines, scrollback);
        this->m_maxScrollPosition = this->m_scrollPosition =
            MAX(0, scrollback - lines);
    }
    else
    {
        this->m_maxScrollPosition = this->m_scrollPosition = 0;
    }
    wresize(m_win, lines, cols);
    scrollRegion(0, lines - 1);
}

std::tuple<int, int> CursesWindow::cursor() const
{
    int y, x;
    getyx(m_win, y, x);
    return {y, x};
}

bool CursesWindow::cursor(int y, int x)
{
    return wmove(m_win, y, x) == OK;
}

void CursesWindow::cup(int y, int x, bool decom)
{
    if (decom)
    {
        throw std::exception();
    }
    wmove(m_win, m_maxScrollPosition + y, x);
}

void CursesWindow::getAttr()
{
    /* save attrs and color pair  */
    wattr_get(m_win, &m_sattr, &m_sp, NULL);
}

void CursesWindow::setAttr()
{
    /* get attrs and color pair  */
    wattr_set(m_win, m_sattr, m_sp, NULL);
}

void CursesWindow::attr(int attr)
{
    wattrset(m_win, attr);
}

void CursesWindow::color(short color, void *opts)
{
    wcolor_set(m_win, color, opts);
}

void CursesWindow::bkg(chtype ch)
{
    wbkgdset(m_win, ch);
}

void CursesWindow::bkg(const cchar_t *ch)
{
    wbkgrndset(m_win, ch);
}

void CursesWindow::setColor(short fg, short bg)
{
    int cp = mtm_alloc_pair(fg, bg);
    wcolor_set(m_win, cp, NULL);
    cchar_t c;
    setcchar(&c, L" ", A_NORMAL, cp, NULL);
    wbkgrndset(m_win, &c);
}

void CursesWindow::scr(int y)
{
    wscrl(m_win, y);
}

bool CursesWindow::scrollRegion(int top, int bottom)
{
    return wsetscrreg(m_win, top, bottom) == OK;
}

std::tuple<int, int> CursesWindow::scrollRegion() const
{
    int top = 0, bot = 0;
    wgetscrreg(m_win, &top, &bot);
    return {top, bot};
}

void CursesWindow::scrollbottom()
{
    this->m_scrollPosition = this->m_maxScrollPosition;
}

void CursesWindow::scrollback(int h)
{
    this->m_scrollPosition = MAX(0, this->m_scrollPosition - h / 2);
}

void CursesWindow::scrollforward(int h)
{
    this->m_scrollPosition =
        MIN(this->m_maxScrollPosition, this->m_scrollPosition + h / 2);
}

void CursesWindow::refresh(int pminrow, int pmincol, int sminrow, int smincol,
                           int smaxrow, int smaxcol)
{
    pnoutrefresh(m_win, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);
}

CursesInput CursesWindow::input() const
{
    wint_t k = 0;
    int r = wget_wch(m_win, &k);
    return {r, k};
}

std::tuple<int, int, int, int, int, int, int, int, int>
CursesWindow::output() const
{
    // COMMONVARS     - All of the common variables for a handler.
    //                  x, y     - cursor position
    //                  mx, my   - max possible values for x and y
    //                  px, py   - physical cursor position in scrollback
    //                  n        - the current node
    //                  m_win      - the current window
    //                  top, bot - the scrolling region
    //                  tos      - top of the screen in the pad
    //                  s        - the current SCRN buffer

    int py, px, y, x, my, mx, top = 0, bot = 0;
    getyx(m_win, py, px);
    y = py - m_maxScrollPosition;
    x = px;
    getmaxyx(m_win, my, mx);
    my -= m_maxScrollPosition;
    wgetscrreg(m_win, &top, &bot);
    bot++;
    bot -= m_maxScrollPosition;
    top = top <= m_maxScrollPosition ? 0 : top - m_maxScrollPosition;

    return {py, px, y, x, my, mx, top, bot, m_maxScrollPosition};
}

void CursesWindow::fixcursor(
    int h, bool focus) /* Move the terminal cursor to the active view. */
{
    if (!focus)
    {
        return;
    }
    curs_set(this->m_scrollPosition == this->m_maxScrollPosition ? this->vis
                                                                 : 0);

    int y, x;
    std::tie(y, x) = cursor();
    cursor(MIN(MAX(y, this->m_maxScrollPosition),
               this->m_maxScrollPosition + h - 1),
           x);
}

void CursesWindow::save()
{
    int y, x;
    std::tie(y, x) = cursor();
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
    cursor(this->sy, this->sx); /* get old position          */
    this->setAttr();
    this->fg = this->sfg;     /* get foreground color      */
    this->bg = this->sbg;     /* get background color      */
    this->xenl = this->oxenl; /* get xenl state            */

    /* restore colors */
    setColor(this->fg, this->bg);

    return true;
}

void CursesWindow::reset(int lines, int scrollback)
{
    if (scrollback)
    {
        lines = MAX(scrollback, lines);
    }
    scrollRegion(0, lines - 1);
    this->vis = 1;
    this->insert = this->oxenl = this->xenl = false;
}

void CursesWindow::del()
{
    wdelch(m_win);
}

void CursesWindow::ins(const wchar_t *ch, int n)
{
    wins_nwstr(m_win, ch, n);
}

void CursesWindow::add(const chtype *ch, int n)
{
    waddchnstr(m_win, ch, n);
}

void CursesWindow::add(const cchar_t *ch, int n)
{
    wadd_wchnstr(m_win, ch, n);
}

void CursesWindow::add(const wchar_t *ch, int n)
{
    waddnwstr(m_win, ch, n);
}

void CursesWindow::add(const char *p)
{
    waddstr(m_win, p);
}

void CursesWindow::clear_to_eol()
{
    wclrtoeol(m_win);
}

void CursesWindow::clear_to_bottom()
{
    wclrtobot(m_win);
}

void CursesWindow::erase()
{
    werase(m_win);
}
