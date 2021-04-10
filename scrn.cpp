#include "node.h"
#include "scrn.h"
#include "minmax.h"

void SCRN::scrollbottom()
{
    this->off = this->tos;
}

void SCRN::fixcursor(int h) /* Move the terminal cursor to the active view. */
{
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
