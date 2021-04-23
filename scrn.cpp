#include "node.h"
#include "scrn.h"
#include "minmax.h"

#include "pair.h"

SCRN::SCRN(int lines, int cols)
{
    this->win = newpad(lines, cols);

    nodelay(this->win, TRUE);
    scrollok(this->win, TRUE);
    keypad(this->win, TRUE);
}

SCRN::~SCRN()
{
    delwin(this->win);
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
    curs_set(this->off != this->tos ? 0 : this->vis);

    int y, x;
    getyx(this->win, y, x);

    y = MIN(MAX(y, this->tos), this->tos + h - 1);
    wmove(this->win, y, x);
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
    /* save attrs and color pair  */
    wattr_get(win, &this->sattr, &this->sp, NULL);
}

void SCRN::setAttr()
{
    /* get attrs and color pair  */
    wattr_set(win, this->sattr, this->sp, NULL);
}

void SCRN::save()
{
    int py, px;
    getyx(this->win, py, px);
    this->sx = px; /* save X position            */
    this->sy = py; /* save Y position            */
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
    wmove(this->win, this->sy, this->sx); /* get old position          */
    this->setAttr();
    this->fg = this->sfg;     /* get foreground color      */
    this->bg = this->sbg;     /* get background color      */
    this->xenl = this->oxenl; /* get xenl state            */

    /* restore colors */
    int cp = mtm_alloc_pair(this->fg, this->bg);
    wcolor_set(this->win, cp, NULL);
    cchar_t c;
    setcchar(&c, L" ", A_NORMAL, cp, NULL);
    wbkgrndset(this->win, &c);
    return true;
}

void SCRN::reset()
{
    auto s = this;
    s->insert = s->oxenl = s->xenl = false;
}