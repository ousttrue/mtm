#include "vtcontext.h"
#include "node.h"
#include "scrn.h"
#include <curses.h>

std::tuple<int, int, int, int, int, int, int, int, int> VtContext::get() const
{
    NODE *n = (NODE *)this->p;
    auto s = n->term->s;
    WINDOW *win = s->win;

    int py, px, y, x, my, mx, top = 0, bot = 0, tos = s->tos;
    getyx(win, py, px);
    y = py - s->tos;
    x = px;
    getmaxyx(win, my, mx);
    my -= s->tos;
    wgetscrreg(win, &top, &bot);
    bot++;
    bot -= s->tos;
    top = top <= tos ? 0 : top - tos;

    return {py, px, y, x, my, mx, top, bot, tos};
}
