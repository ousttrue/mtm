#include "pair.h"
#include "config.h"
#include "vtcontext.h"
#include "vtparser.h"
#include "vthandler.h"
#include "curses_term.h"
#include "curses_window.h"
#include "minmax.h"
#include <climits>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

//  *      CALL(h, term)        - Call handler h with no arguments.
inline void CALL(VTCALLBACK x, CursesTerm *term)
{
    x({term, 0, 0, 0, NULL});
}

static void cup(VtContext context)
{ /* CUP - Cursor Position */
    auto term = context.term();
    term->s->xenl = false;
    term->s->cup(context.P1(0)-1, context.P1(1)-1, term->decom);
    context.end();
}

static void dch(VtContext context)
{ /* DCH - Delete Character */
    auto term = context.term();
    for (int i = 0; i < context.P1(0); i++)
        wdelch(term->s->win());
    context.end();
}

static void ich(VtContext context)
{ /* ICH - Insert Character */
    auto term = context.term();
    for (int i = 0; i < context.P1(0); i++)
        wins_nwstr(term->s->win(), L" ", 1);
    context.end();
}

static void cuu(VtContext context)
{ /* CUU - Cursor Up */
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    wmove(term->s->win(), MAX(py - context.P1(0), tos + top), x);
    context.end();
}

static void cud(VtContext context)
{ /* CUD - Cursor Down */
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    wmove(term->s->win(), MIN(py + context.P1(0), tos + bot - 1), x);
    context.end();
}

static void cuf(VtContext context)
{ /* CUF - Cursor Forward */
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    wmove(term->s->win(), py, MIN(x + context.P1(0), mx - 1));
    context.end();
}

static void hts(VtContext context)
{ /* HTS - Horizontal Tab Set */
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    term->HorizontalTabSet(x);
    context.end();
}

static void ri(VtContext context)
{ /* RI - Reverse Index */
    auto term = context.term();
    int otop = 0, obot = 0;
    wgetscrreg(term->s->win(), &otop, &obot);
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    wsetscrreg(term->s->win(), otop >= tos ? otop : tos, obot);
    y == top ? wscrl(term->s->win(), -1)
             : wmove(term->s->win(), MAX(tos, py - 1), x);
    wsetscrreg(term->s->win(), otop, obot);
    context.end();
}

static void decid(VtContext context)
{ /* DECID - Send Terminal Identification */
    auto term = context.term();
    if (context.w == L'c')
        term->safewrite(context.iw == L'>' ? "\033[>1;10;0c" : "\033[?1;2c");
    else if (context.w == L'Z')
        term->safewrite("\033[?6c");
    context.end();
}

static void hpa(VtContext context)
{ /* HPA - Cursor Horizontal Absolute */
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    wmove(term->s->win(), py, MIN(context.P1(0) - 1, mx - 1));
    context.end();
}

static void hpr(VtContext context)
{ /* HPR - Cursor Horizontal Relative */
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    wmove(term->s->win(), py, MIN(px + context.P1(0), mx - 1));
    context.end();
}

static void vpa(VtContext context)
{ /* VPA - Cursor Vertical Absolute */
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    wmove(term->s->win(),
          MIN(tos + bot - 1, MAX(tos + top, tos + context.P1(0) - 1)), x);
    context.end();
}

static void vpr(VtContext context)
{ /* VPR - Cursor Vertical Relative */
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    wmove(term->s->win(),
          MIN(tos + bot - 1, MAX(tos + top, py + context.P1(0))), x);
    context.end();
}

static void cbt(VtContext context)
{ /* CBT - Cursor Backwards Tab */
    int i;
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    if (term->TryGetBackwardTab(x, &i))
    {
        wmove(term->s->win(), py, i);
    }
    wmove(term->s->win(), py, 0);
    context.end();
}

static void ht(VtContext context)
{ /* HT - Horizontal Tab */
    auto term = context.term();
    int i;
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    if (term->TryGetForwardTab(x, &i))
    {
        wmove(term->s->win(), py, i);
    }
    wmove(term->s->win(), py, mx - 1);
    context.end();
}

static void tab(VtContext context)
{ /* Tab forwards or backwards */
    auto term = context.term();
    for (int i = 0; i < context.P1(0); i++)
        switch (context.w)
        {
        case L'I':
            ht({term, 0, 0, 0, nullptr});
            break;
        case L'\t':
            CALL(ht, term);
            break;
        case L'Z':
            CALL(cbt, term);
            break;
        }
    context.end();
}

static void decaln(VtContext context)
{ /* DECALN - Screen Alignment Test */
    auto term = context.term();
    chtype e[] = {COLOR_PAIR(0) | 'E', 0};
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    for (int r = 0; r < my; r++)
    {
        for (int c = 0; c <= mx; c++)
            mvwaddchnstr(term->s->win(), tos + r, c, e, 1);
    }
    wmove(term->s->win(), py, px);
    context.end();
}

static void su(VtContext context)
{ /* SU - Scroll Up/Down */
    auto term = context.term();
    wscrl(term->s->win(), (context.w == L'T' || context.w == L'^')
                              ? -context.P1(0)
                              : context.P1(0));
    context.end();
}

static void sc(VtContext context)
{ /* SC - Save Cursor */
    auto term = context.term();
    auto s = term->s;
    s->save();

    term->sgc = term->gc;
    term->sgs = term->gs; /* save character sets        */
    context.end();
}

static void rc(VtContext context)
{ /* RC - Restore Cursor */
    auto term = context.term();
    if (context.iw == L'#')
    {
        CALL(decaln, term);
        return;
    }

    if (!term->s->restore())
    {
        return;
    }
    term->gc = term->sgc;
    term->gs = term->sgs; /* save character sets        */

    context.end();
}

static void tbc(VtContext context)
{ /* TBC - Tabulation Clear */
    auto term = context.term();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    switch (context.P0(0))
    {
    case 0:
        term->TabClear(x);
        break;

    case 3:
        term->TabClearAll();
        break;
    }
    context.end();
}

static void cub(VtContext context)
{ /* CUB - Cursor Backward */
    auto term = context.term();
    auto s = term->s;
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    s->xenl = false;
    wmove(s->win(), py, MAX(x - context.P1(0), 0));
    context.end();
}

static void el(VtContext context)
{ /* EL - Erase in Line */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    cchar_t b;
    setcchar(&b, L" ", A_NORMAL, mtm_alloc_pair(s->fg, s->bg), NULL);
    switch (context.P0(0))
    {
    case 0:
        wclrtoeol(win);
        break;
    case 1:
        for (int i = 0; i <= x; i++)
            mvwadd_wchnstr(win, py, i, &b, 1);
        break;
    case 2:
        wmove(win, py, 0);
        wclrtoeol(win);
        break;
    }
    wmove(win, py, x);
    context.end();
}

static void ed(VtContext context)
{ /* ED - Erase in Display */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    int o = 1;
    switch (context.P0(0))
    {
    case 0:
        wclrtobot(win);
        break;
    case 3:
        werase(win);
        break;
    case 2:
        wmove(win, tos, 0);
        wclrtobot(win);
        break;
    case 1:
        for (int i = tos; i < py; i++)
        {
            wmove(win, i, 0);
            wclrtoeol(win);
        }
        wmove(win, py, x);
        el({context.p, context.w, context.iw, 1, &o});
        break;
    }
    wmove(win, py, px);
    context.end();
}

static void ech(VtContext context)
{ /* ECH - Erase Character */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    cchar_t c;
    setcchar(&c, L" ", A_NORMAL, mtm_alloc_pair(s->fg, s->bg), NULL);
    for (int i = 0; i < context.P1(0); i++)
        mvwadd_wchnstr(win, py, x + i, &c, 1);
    wmove(win, py, px);
    context.end();
}

static void dsr(VtContext context)
{ /* DSR - Device Status Report */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    char buf[100] = {0};
    if (context.P0(0) == 6)
        snprintf(buf, sizeof(buf) - 1, "\033[%d;%dR",
                 (term->decom ? y - top : y) + 1, x + 1);
    else
        snprintf(buf, sizeof(buf) - 1, "\033[0n");
    term->safewrite(buf);
    context.end();
}

static void idl(VtContext context)
{ /* IL or DL - Insert/Delete Line */
    /* we don't use insdelln here because it inserts above and not below,
     * and has a few other edge cases... */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    int otop = 0, obot = 0, p1 = MIN(context.P1(0), (my - 1) - y);
    wgetscrreg(win, &otop, &obot);
    wsetscrreg(win, py, obot);
    wscrl(win, context.w == L'L' ? -p1 : p1);
    wsetscrreg(win, otop, obot);
    wmove(win, py, 0);
    context.end();
}

static void csr(VtContext context)
{ /* CSR - Change Scrolling Region */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    if (wsetscrreg(win, tos + context.P1(0) - 1, tos + context.PD(1, my) - 1) ==
        OK)
        CALL(cup, term);
    context.end();
}

static void decreqtparm(VtContext context)
{ /* DECREQTPARM - Request Device Parameters */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    term->safewrite(context.P0(0) ? "\033[3;1;2;120;1;0x"
                                  : "\033[2;1;2;120;128;1;0x");
    context.end();
}

static void sgr0(VtContext context)
{ /* Reset SGR to default */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();
    wattrset(win, A_NORMAL);
    wcolor_set(win, 0, NULL);
    s->fg = s->bg = -1;
    wbkgdset(win, COLOR_PAIR(0) | ' ');
    context.end();
}

static void cls(VtContext context)
{ /* Clear screen */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    CALL(cup, term);
    wclrtobot(win);
    CALL(cup, term);
    context.end();
}

static void ris(VtContext context)
{ /* RIS - Reset to Initial State */
    auto term = context.term();
    CALL(cls, term);
    CALL(sgr0, term);
    term->reset();
    context.end();
}

static void mode(VtContext context)
{ /* Set or Reset Mode */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    bool set = (context.w == L'h');
    for (int i = 0; i < context.argc; i++)
        switch (context.P0(i))
        {
        case 1:
            term->pnm = set;
            break;
        case 3:
            CALL(cls, term);
            break;
        case 4:
            s->insert = set;
            break;
        case 6:
            term->decom = set;
            CALL(cup, term);
            break;
        case 7:
            term->am = set;
            break;
        case 20:
            term->lnm = set;
            break;
        case 25:
            s->vis = set ? 1 : 0;
            break;
        case 34:
            s->vis = set ? 1 : 2;
            break;
        case 1048:
            CALL((set ? sc : rc), term);
            break;
        case 1049:
            CALL((set ? sc : rc), term); /* fall-through */
        case 47:
        case 1047:
            if (term->alternate_screen_buffer_mode(set))
            {
                CALL(cls, term);
            }
            break;
        }
    context.end();
}

static void sgr(VtContext context)
{ /* SGR - Select Graphic Rendition */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();

    bool doc = false, do8 = COLORS >= 8, do16 = COLORS >= 16,
         do256 = COLORS >= 256;
    if (!context.argc)
        CALL(sgr0, term);

    short bg = s->bg, fg = s->fg;
    for (int i = 0; i < context.argc; i++)
        switch (context.P0(i))
        {
        case 0:
            CALL(sgr0, term);
            break;
        case 1:
            wattron(win, A_BOLD);
            break;
        case 2:
            wattron(win, A_DIM);
            break;
        case 4:
            wattron(win, A_UNDERLINE);
            break;
        case 5:
            wattron(win, A_BLINK);
            break;
        case 7:
            wattron(win, A_REVERSE);
            break;
        case 8:
            wattron(win, A_INVIS);
            break;
        case 22:
            wattroff(win, A_DIM);
            wattroff(win, A_BOLD);
            break;
        case 24:
            wattroff(win, A_UNDERLINE);
            break;
        case 25:
            wattroff(win, A_BLINK);
            break;
        case 27:
            wattroff(win, A_REVERSE);
            break;
        case 30:
            fg = COLOR_BLACK;
            doc = do8;
            break;
        case 31:
            fg = COLOR_RED;
            doc = do8;
            break;
        case 32:
            fg = COLOR_GREEN;
            doc = do8;
            break;
        case 33:
            fg = COLOR_YELLOW;
            doc = do8;
            break;
        case 34:
            fg = COLOR_BLUE;
            doc = do8;
            break;
        case 35:
            fg = COLOR_MAGENTA;
            doc = do8;
            break;
        case 36:
            fg = COLOR_CYAN;
            doc = do8;
            break;
        case 37:
            fg = COLOR_WHITE;
            doc = do8;
            break;
        case 38:
            fg = context.P0(i + 1) == 5 ? context.P0(i + 2) : s->fg;
            i += 2;
            doc = do256;
            break;
        case 39:
            fg = -1;
            doc = true;
            break;
        case 40:
            bg = COLOR_BLACK;
            doc = do8;
            break;
        case 41:
            bg = COLOR_RED;
            doc = do8;
            break;
        case 42:
            bg = COLOR_GREEN;
            doc = do8;
            break;
        case 43:
            bg = COLOR_YELLOW;
            doc = do8;
            break;
        case 44:
            bg = COLOR_BLUE;
            doc = do8;
            break;
        case 45:
            bg = COLOR_MAGENTA;
            doc = do8;
            break;
        case 46:
            bg = COLOR_CYAN;
            doc = do8;
            break;
        case 47:
            bg = COLOR_WHITE;
            doc = do8;
            break;
        case 48:
            bg = context.P0(i + 1) == 5 ? context.P0(i + 2) : s->bg;
            i += 2;
            doc = do256;
            break;
        case 49:
            bg = -1;
            doc = true;
            break;
        case 90:
            fg = COLOR_BLACK;
            doc = do16;
            break;
        case 91:
            fg = COLOR_RED;
            doc = do16;
            break;
        case 92:
            fg = COLOR_GREEN;
            doc = do16;
            break;
        case 93:
            fg = COLOR_YELLOW;
            doc = do16;
            break;
        case 94:
            fg = COLOR_BLUE;
            doc = do16;
            break;
        case 95:
            fg = COLOR_MAGENTA;
            doc = do16;
            break;
        case 96:
            fg = COLOR_CYAN;
            doc = do16;
            break;
        case 97:
            fg = COLOR_WHITE;
            doc = do16;
            break;
        case 100:
            bg = COLOR_BLACK;
            doc = do16;
            break;
        case 101:
            bg = COLOR_RED;
            doc = do16;
            break;
        case 102:
            bg = COLOR_GREEN;
            doc = do16;
            break;
        case 103:
            bg = COLOR_YELLOW;
            doc = do16;
            break;
        case 104:
            bg = COLOR_BLUE;
            doc = do16;
            break;
        case 105:
            bg = COLOR_MAGENTA;
            doc = do16;
            break;
        case 106:
            bg = COLOR_CYAN;
            doc = do16;
            break;
        case 107:
            bg = COLOR_WHITE;
            doc = do16;
            break;
#if defined(A_ITALIC) && !defined(NO_ITALICS)
        case 3:
            wattron(win, A_ITALIC);
            break;
        case 23:
            wattroff(win, A_ITALIC);
            break;
#endif
        }
    if (doc)
    {
        int p = mtm_alloc_pair(s->fg = fg, s->bg = bg);
        wcolor_set(win, p, NULL);
        cchar_t c;
        setcchar(&c, L" ", A_NORMAL, p, NULL);
        wbkgrndset(win, &c);
    }
}

static void cr(VtContext context)
{ /* CR - Carriage Return */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();

    s->xenl = false;
    wmove(win, py, 0);
    context.end();
}

static void ind(VtContext context)
{ /* IND - Index */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();

    y == (bot - 1) ? scroll(win) : wmove(win, py + 1, x);
    context.end();
}

static void nel(VtContext context)
{ /* NEL - Next Line */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    CALL(cr, term);
    CALL(ind, term);
    context.end();
}

static void pnl(VtContext context)
{ /* NL - Newline */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    CALL((term->lnm ? nel : ind), term);
    context.end();
}

static void cpl(VtContext context)
{ /* CPL - Cursor Previous Line */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();

    wmove(win, MAX(tos + top, py - context.P1(0)), 0);
    context.end();
}

static void cnl(VtContext context)
{ /* CNL - Cursor Next Line */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();

    wmove(win, MIN(tos + bot - 1, py + context.P1(0)), 0);
    context.end();
}

static void print(VtContext context)
{ /* Print a character to the terminal */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();

    if (wcwidth(context.w) < 0)
        return;

    if (s->insert)
        CALL(ich, term);

    if (s->xenl)
    {
        s->xenl = false;
        if (term->am)
            CALL(nel, term);
        getyx(win, y, x);
        y -= tos;
    }

    if (context.w < MAXMAP && term->gc[context.w])
        context.w = term->gc[context.w];
    term->repc = context.w;

    if (x == mx - wcwidth(context.w))
    {
        s->xenl = true;
        wins_nwstr(win, &context.w, 1);
    }
    else
        waddnwstr(win, &context.w, 1);
    term->gc = term->gs;
} /* no context.end(); } because we don't want to reset repc */

static void rep(VtContext context)
{ /* REP - Repeat Character */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();

    for (int i = 0; i < context.P1(0) && term->repc; i++)
        print({context.p, term->repc, 0, 0, NULL});
    context.end();
}

static void scs(VtContext context)
{ /* Select Character Set */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();

    wchar_t **t = NULL;
    switch (context.iw)
    {
    case L'(':
        t = &term->g0;
        break;
    case L')':
        t = &term->g1;
        break;
    case L'*':
        t = &term->g2;
        break;
    case L'+':
        t = &term->g3;
        break;
    default:
        return;
        break;
    }
    switch (context.w)
    {
    case L'A':
        *t = CSET_UK;
        break;
    case L'B':
        *t = CSET_US;
        break;
    case L'0':
        *t = CSET_GRAPH;
        break;
    case L'1':
        *t = CSET_US;
        break;
    case L'2':
        *t = CSET_GRAPH;
        break;
    }
    context.end();
}

static void so(VtContext context)
{ /* Switch Out/In Character Set */
    auto term = context.term();
    auto s = term->s;
    auto win = s->win();
    int py, px, y, x, my, mx, top, bot, tos;
    std::tie(py, px, y, x, my, mx, top, bot, tos) = term->s->output();

    if (context.w == 0x0e)
        term->gs = term->gc = term->g1; /* locking shift */
    else if (context.w == 0xf)
        term->gs = term->gc = term->g0; /* locking shift */
    else if (context.w == L'n')
        term->gs = term->gc = term->g2; /* locking shift */
    else if (context.w == L'o')
        term->gs = term->gc = term->g3; /* locking shift */
    else if (context.w == L'N')
    {
        term->gs = term->gc; /* non-locking shift */
        term->gc = term->g2;
    }
    else if (context.w == L'O')
    {
        term->gs = term->gc; /* non-locking shift */
        term->gc = term->g3;
    }
    context.end();
}

static void setupevents(const std::unique_ptr<VtParser> &vp)
{
    /* ACK - Acknowledge Enquiry */
    vp->setControl(ControlCodes::ENQ, [](VtContext ctx) {
        auto term = ctx.term();
        term->safewrite("\006");
        ctx.end();
    });

    /* Terminal bell. */
    vp->setControl(ControlCodes::BEL, [](VtContext ctx) {
        beep();
        ctx.end();
    });

    vp->setControl(ControlCodes::BS, cub);
    vp->setControl(ControlCodes::HT, tab);
    vp->setControl(ControlCodes::LF, pnl);
    vp->setControl(ControlCodes::VT, pnl);
    vp->setControl(ControlCodes::FF, pnl);
    vp->setControl(ControlCodes::CR, cr);
    vp->setControl(ControlCodes::SO, so);
    vp->setControl(ControlCodes::SI, so);

    // 0x1B [ ...
    vp->setCsi(L'A', cuu);
    vp->setCsi(L'B', cud);
    vp->setCsi(L'C', cuf);
    vp->setCsi(L'D', cub);
    vp->setCsi(L'E', cnl);
    vp->setCsi(L'F', cpl);
    vp->setCsi(L'G', hpa);
    vp->setCsi(L'H', cup);
    vp->setCsi(L'I', tab);
    vp->setCsi(L'J', ed);
    vp->setCsi(L'K', el);
    vp->setCsi(L'L', idl);
    vp->setCsi(L'M', idl);
    vp->setCsi(L'P', dch);
    vp->setCsi(L'S', su);
    vp->setCsi(L'T', su);
    vp->setCsi(L'X', ech);
    vp->setCsi(L'Z', tab);
    vp->setCsi(L'`', hpa);
    vp->setCsi(L'^', su);
    vp->setCsi(L'@', ich);
    vp->setCsi(L'a', hpr);
    vp->setCsi(L'b', rep);
    vp->setCsi(L'c', decid);
    vp->setCsi(L'd', vpa);
    vp->setCsi(L'e', vpr);
    vp->setCsi(L'f', cup);
    vp->setCsi(L'g', tbc);
    vp->setCsi(L'h', mode);
    vp->setCsi(L'l', mode);
    vp->setCsi(L'm', sgr);
    vp->setCsi(L'n', dsr);
    vp->setCsi(L'r', csr);
    vp->setCsi(L's', sc);
    vp->setCsi(L'u', rc);
    vp->setCsi(L'x', decreqtparm);

    vp->setEscape(L'0', scs);
    vp->setEscape(L'1', scs);
    vp->setEscape(L'2', scs);
    vp->setEscape(L'7', sc);
    vp->setEscape(L'8', rc);
    vp->setEscape(L'A', scs);
    vp->setEscape(L'B', scs);
    vp->setEscape(L'D', ind);
    vp->setEscape(L'E', nel);
    vp->setEscape(L'H', hts);
    vp->setEscape(L'M', ri);
    vp->setEscape(L'Z', decid);
    vp->setEscape(L'c', ris);

    /* Cursor visibility */
    vp->setEscape(L'p', [](VtContext ctx) {
        ctx.term()->s->vis = ctx.iw == L'6' ? 0 : 1;
        ctx.end();
    });

    vp->setEscape(L'=', [](VtContext context) {
        /* Application/Numeric Keypad Mode */
        context.term()->pnm = (context.w == L'=');
        context.end();
    });
    vp->setEscape(L'>', [](VtContext context) {
        /* Application/Numeric Keypad Mode */
        context.term()->pnm = (context.w == L'=');
        context.end();
    });
    vp->setPrint(print);
}

void vp_initialize(CursesTerm *term)
{
    setupevents(term->vp);
    ris({term, L'c', 0, 0, NULL});
}
