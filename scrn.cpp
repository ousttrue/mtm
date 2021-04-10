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
