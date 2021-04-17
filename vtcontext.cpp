#include "vtcontext.h"
#include "curses_term.h"
#include "node.h"
#include "scrn.h"
#include <curses.h>

// (END)HANDLER   - Declare/end a handler function
 void VtContext::end()
{
    auto term = (CursesTerm *)this->p;
    term->repc = 0;
}

std::tuple<int, int, int, int, int, int, int, int, int> VtContext::get() const
{
    auto term = (CursesTerm *)this->p;
    auto s = term->s;
    WINDOW *win = s->win;

    // COMMONVARS     - All of the common variables for a handler.
    //                  x, y     - cursor position
    //                  mx, my   - max possible values for x and y
    //                  px, py   - physical cursor position in scrollback
    //                  n        - the current node
    //                  win      - the current window
    //                  top, bot - the scrolling region
    //                  tos      - top of the screen in the pad
    //                  s        - the current SCRN buffer

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
