#include "pair.h"
#include "config.h"
#include "global.h"
#include "vtparser.h"
#include "vthandler.h"
#include "node.h"
#include "curses_term.h"
#include "scrn.h"
#include "minmax.h"
#include <climits>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

/*** TERMINAL EMULATION HANDLERS
 * These functions implement the various terminal commands activated by
 * escape sequences and printing to the terminal. Large amounts of boilerplate
 * code is shared among all these functions, and is factored out into the
 * macros below:
 *      PD(n, d)       - Parameter n, with default d.
 *      P0(n)          - Parameter n, default 0.
 *      P1(n)          - Parameter n, default 1.
 *      CALL(h)        - Call handler h with no arguments.
 *      SENDN(n, s, c) - Write string c bytes of s to n.
 *      n->term->safewrite( s)     - Write string s to node n's host.
 *      (END)HANDLER   - Declare/end a handler function
 *      COMMONVARS     - All of the common variables for a handler.
 *                       x, y     - cursor position
 *                       mx, my   - max possible values for x and y
 *                       px, py   - physical cursor position in scrollback
 *                       n        - the current node
 *                       win      - the current window
 *                       top, bot - the scrolling region
 *                       tos      - top of the screen in the pad
 *                       s        - the current SCRN buffer
 * The funny names for handlers are from their ANSI/ECMA/DEC mnemonics.
 */

#define PD(x, d) (argc < (x) || !argv ? (d) : argv[(x)])
#define P0(x) PD(x, 0)
#define P1(x) (!P0(x) ? 1 : P0(x))
#define CALL(x) (x)(n, 0, 0, 0, NULL, NULL)

#define COMMONVARS                                                             \
    NODE *n = (NODE *)p;                                                       \
    auto s = n->term->s;                                                         \
    WINDOW *win = s->win;                                                      \
    int py, px, y, x, my, mx, top = 0, bot = 0, tos = s->tos;                  \
    (void)p;                                                                   \
    (void)w;                                                                   \
    (void)iw;                                                                  \
    (void)argc;                                                                \
    (void)argv;                                                                \
    (void)win;                                                                 \
    (void)y;                                                                   \
    (void)x;                                                                   \
    (void)my;                                                                  \
    (void)mx;                                                                  \
    (void)osc;                                                                 \
    (void)tos;                                                                 \
    getyx(win, py, px);                                                        \
    y = py - s->tos;                                                           \
    x = px;                                                                    \
    getmaxyx(win, my, mx);                                                     \
    my -= s->tos;                                                              \
    wgetscrreg(win, &top, &bot);                                               \
    bot++;                                                                     \
    bot -= s->tos;                                                             \
    top = top <= tos ? 0 : top - tos;

#define HANDLER(name)                                                          \
    static void name(void *p, wchar_t w, wchar_t iw, int argc, int *argv,      \
                     const wchar_t *osc)                                       \
    {                                                                          \
        COMMONVARS
#define ENDHANDLER                                                             \
    n->term->repc = 0;                                                           \
    } /* control sequences aren't repeated */

HANDLER(bell) /* Terminal bell. */
beep();
ENDHANDLER

HANDLER(numkp) /* Application/Numeric Keypad Mode */
n->term->pnm = (w == L'=');
ENDHANDLER

HANDLER(vis) /* Cursor visibility */
s->vis = iw == L'6' ? 0 : 1;
ENDHANDLER

HANDLER(cup) /* CUP - Cursor Position */
s->xenl = false;
wmove(win, tos + (n->term->decom ? top : 0) + P1(0) - 1, P1(1) - 1);
ENDHANDLER

HANDLER(dch) /* DCH - Delete Character */
for (int i = 0; i < P1(0); i++)
    wdelch(win);
ENDHANDLER

HANDLER(ich) /* ICH - Insert Character */
for (int i = 0; i < P1(0); i++)
    wins_nwstr(win, L" ", 1);
ENDHANDLER

HANDLER(cuu) /* CUU - Cursor Up */
wmove(win, MAX(py - P1(0), tos + top), x);
ENDHANDLER

HANDLER(cud) /* CUD - Cursor Down */
wmove(win, MIN(py + P1(0), tos + bot - 1), x);
ENDHANDLER

HANDLER(cuf) /* CUF - Cursor Forward */
wmove(win, py, MIN(x + P1(0), mx - 1));
ENDHANDLER

HANDLER(ack) /* ACK - Acknowledge Enquiry */
n->term->safewrite("\006");
ENDHANDLER

HANDLER(hts) /* HTS - Horizontal Tab Set */
n->term->HorizontalTabSet(x);
ENDHANDLER

HANDLER(ri) /* RI - Reverse Index */
int otop = 0, obot = 0;
wgetscrreg(win, &otop, &obot);
wsetscrreg(win, otop >= tos ? otop : tos, obot);
y == top ? wscrl(win, -1) : wmove(win, MAX(tos, py - 1), x);
wsetscrreg(win, otop, obot);
ENDHANDLER

HANDLER(decid) /* DECID - Send Terminal Identification */
if (w == L'c')
    n->term->safewrite(iw == L'>' ? "\033[>1;10;0c" : "\033[?1;2c");
else if (w == L'Z')
    n->term->safewrite("\033[?6c");
ENDHANDLER

HANDLER(hpa) /* HPA - Cursor Horizontal Absolute */
wmove(win, py, MIN(P1(0) - 1, mx - 1));
ENDHANDLER

HANDLER(hpr) /* HPR - Cursor Horizontal Relative */
wmove(win, py, MIN(px + P1(0), mx - 1));
ENDHANDLER

HANDLER(vpa) /* VPA - Cursor Vertical Absolute */
wmove(win, MIN(tos + bot - 1, MAX(tos + top, tos + P1(0) - 1)), x);
ENDHANDLER

HANDLER(vpr) /* VPR - Cursor Vertical Relative */
wmove(win, MIN(tos + bot - 1, MAX(tos + top, py + P1(0))), x);
ENDHANDLER

HANDLER(cbt) /* CBT - Cursor Backwards Tab */
int i;
if (n->term->TryGetBackwardTab(x, &i))
{
    wmove(win, py, i);
}
wmove(win, py, 0);
ENDHANDLER

HANDLER(ht) /* HT - Horizontal Tab */
int i;
if (n->term->TryGetForwardTab(x, &i))
{
    wmove(win, py, i);
}
wmove(win, py, mx - 1);
ENDHANDLER

HANDLER(tab) /* Tab forwards or backwards */
for (int i = 0; i < P1(0); i++)
    switch (w)
    {
    case L'I':
        CALL(ht);
        break;
    case L'\t':
        CALL(ht);
        break;
    case L'Z':
        CALL(cbt);
        break;
    }
ENDHANDLER

HANDLER(decaln) /* DECALN - Screen Alignment Test */
chtype e[] = {COLOR_PAIR(0) | 'E', 0};
for (int r = 0; r < my; r++)
{
    for (int c = 0; c <= mx; c++)
        mvwaddchnstr(win, tos + r, c, e, 1);
}
wmove(win, py, px);
ENDHANDLER

HANDLER(su) /* SU - Scroll Up/Down */
wscrl(win, (w == L'T' || w == L'^') ? -P1(0) : P1(0));
ENDHANDLER

HANDLER(sc) /* SC - Save Cursor */
s->sx = px; /* save X position            */
s->sy = py; /* save Y position            */
s->getAttr();
s->sfg = s->fg;     /* save foreground color      */
s->sbg = s->bg;     /* save background color      */
s->oxenl = s->xenl; /* save xenl state            */
s->saved = true;    /* save data is valid         */
n->term->sgc = n->term->gc;
n->term->sgs = n->term->gs; /* save character sets        */
ENDHANDLER

HANDLER(rc) /* RC - Restore Cursor */
if (iw == L'#')
{
    CALL(decaln);
    return;
}
if (!s->saved)
    return;
wmove(win, s->sy, s->sx); /* get old position          */
s->setAttr();
s->fg = s->sfg;     /* get foreground color      */
s->bg = s->sbg;     /* get background color      */
s->xenl = s->oxenl; /* get xenl state            */
n->term->gc = n->term->sgc;
n->term->gs = n->term->sgs; /* save character sets        */

/* restore colors */
int cp = mtm_alloc_pair(s->fg, s->bg);
wcolor_set(win, cp, NULL);
cchar_t c;
setcchar(&c, L" ", A_NORMAL, cp, NULL);
wbkgrndset(win, &c);
ENDHANDLER

HANDLER(tbc) /* TBC - Tabulation Clear */
switch (P0(0))
{
case 0:
    n->term->TabClear(x);
    break;

case 3:
    n->term->TabClearAll();
    break;
}
ENDHANDLER

HANDLER(cub) /* CUB - Cursor Backward */
s->xenl = false;
wmove(win, py, MAX(x - P1(0), 0));
ENDHANDLER

HANDLER(el) /* EL - Erase in Line */
cchar_t b;
setcchar(&b, L" ", A_NORMAL, mtm_alloc_pair(s->fg, s->bg), NULL);
switch (P0(0))
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
ENDHANDLER

HANDLER(ed) /* ED - Erase in Display */
int o = 1;
switch (P0(0))
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
    el(p, w, iw, 1, &o, NULL);
    break;
}
wmove(win, py, px);
ENDHANDLER

HANDLER(ech) /* ECH - Erase Character */
cchar_t c;
setcchar(&c, L" ", A_NORMAL, mtm_alloc_pair(s->fg, s->bg), NULL);
for (int i = 0; i < P1(0); i++)
    mvwadd_wchnstr(win, py, x + i, &c, 1);
wmove(win, py, px);
ENDHANDLER

HANDLER(dsr) /* DSR - Device Status Report */
char buf[100] = {0};
if (P0(0) == 6)
    snprintf(buf, sizeof(buf) - 1, "\033[%d;%dR",
             (n->term->decom ? y - top : y) + 1, x + 1);
else
    snprintf(buf, sizeof(buf) - 1, "\033[0n");
n->term->safewrite(buf);
ENDHANDLER

HANDLER(idl) /* IL or DL - Insert/Delete Line */
/* we don't use insdelln here because it inserts above and not below,
 * and has a few other edge cases... */
int otop = 0, obot = 0, p1 = MIN(P1(0), (my - 1) - y);
wgetscrreg(win, &otop, &obot);
wsetscrreg(win, py, obot);
wscrl(win, w == L'L' ? -p1 : p1);
wsetscrreg(win, otop, obot);
wmove(win, py, 0);
ENDHANDLER

HANDLER(csr) /* CSR - Change Scrolling Region */
if (wsetscrreg(win, tos + P1(0) - 1, tos + PD(1, my) - 1) == OK)
    CALL(cup);
ENDHANDLER

HANDLER(decreqtparm) /* DECREQTPARM - Request Device Parameters */
n->term->safewrite(P0(0) ? "\033[3;1;2;120;1;0x" : "\033[2;1;2;120;128;1;0x");
ENDHANDLER

HANDLER(sgr0) /* Reset SGR to default */
wattrset(win, A_NORMAL);
wcolor_set(win, 0, NULL);
s->fg = s->bg = -1;
wbkgdset(win, COLOR_PAIR(0) | ' ');
ENDHANDLER

HANDLER(cls) /* Clear screen */
CALL(cup);
wclrtobot(win);
CALL(cup);
ENDHANDLER

HANDLER(ris) /* RIS - Reset to Initial State */
CALL(cls);
CALL(sgr0);
n->term->reset();
ENDHANDLER

HANDLER(mode) /* Set or Reset Mode */
bool set = (w == L'h');
for (int i = 0; i < argc; i++)
    switch (P0(i))
    {
    case 1:
        n->term->pnm = set;
        break;
    case 3:
        CALL(cls);
        break;
    case 4:
        s->insert = set;
        break;
    case 6:
        n->term->decom = set;
        CALL(cup);
        break;
    case 7:
        n->term->am = set;
        break;
    case 20:
        n->term->lnm = set;
        break;
    case 25:
        s->vis = set ? 1 : 0;
        break;
    case 34:
        s->vis = set ? 1 : 2;
        break;
    case 1048:
        CALL((set ? sc : rc));
        break;
    case 1049:
        CALL((set ? sc : rc)); /* fall-through */
    case 47:
    case 1047:
        if (n->term->alternate_screen_buffer_mode(set))
        {
            CALL(cls);
        }
        break;
    }
ENDHANDLER

HANDLER(sgr) /* SGR - Select Graphic Rendition */
bool doc = false, do8 = COLORS >= 8, do16 = COLORS >= 16, do256 = COLORS >= 256;
if (!argc)
    CALL(sgr0);

short bg = s->bg, fg = s->fg;
for (int i = 0; i < argc; i++)
    switch (P0(i))
    {
    case 0:
        CALL(sgr0);
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
        fg = P0(i + 1) == 5 ? P0(i + 2) : s->fg;
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
        bg = P0(i + 1) == 5 ? P0(i + 2) : s->bg;
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

HANDLER(cr) /* CR - Carriage Return */
s->xenl = false;
wmove(win, py, 0);
ENDHANDLER

HANDLER(ind) /* IND - Index */
y == (bot - 1) ? scroll(win) : wmove(win, py + 1, x);
ENDHANDLER

HANDLER(nel) /* NEL - Next Line */
CALL(cr);
CALL(ind);
ENDHANDLER

HANDLER(pnl) /* NL - Newline */
CALL((n->term->lnm ? nel : ind));
ENDHANDLER

HANDLER(cpl) /* CPL - Cursor Previous Line */
wmove(win, MAX(tos + top, py - P1(0)), 0);
ENDHANDLER

HANDLER(cnl) /* CNL - Cursor Next Line */
wmove(win, MIN(tos + bot - 1, py + P1(0)), 0);
ENDHANDLER

HANDLER(print) /* Print a character to the terminal */
if (wcwidth(w) < 0)
    return;

if (s->insert)
    CALL(ich);

if (s->xenl)
{
    s->xenl = false;
    if (n->term->am)
        CALL(nel);
    getyx(win, y, x);
    y -= tos;
}

if (w < MAXMAP && n->term->gc[w])
    w = n->term->gc[w];
n->term->repc = w;

if (x == mx - wcwidth(w))
{
    s->xenl = true;
    wins_nwstr(win, &w, 1);
}
else
    waddnwstr(win, &w, 1);
n->term->gc = n->term->gs;
} /* no ENDHANDLER because we don't want to reset repc */

HANDLER(rep) /* REP - Repeat Character */
for (int i = 0; i < P1(0) && n->term->repc; i++)
    print(p, n->term->repc, 0, 0, NULL, NULL);
ENDHANDLER

HANDLER(scs) /* Select Character Set */
wchar_t **t = NULL;
switch (iw)
{
case L'(':
    t = &n->term->g0;
    break;
case L')':
    t = &n->term->g1;
    break;
case L'*':
    t = &n->term->g2;
    break;
case L'+':
    t = &n->term->g3;
    break;
default:
    return;
    break;
}
switch (w)
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
ENDHANDLER

HANDLER(so) /* Switch Out/In Character Set */
if (w == 0x0e)
    n->term->gs = n->term->gc = n->term->g1; /* locking shift */
else if (w == 0xf)
    n->term->gs = n->term->gc = n->term->g0; /* locking shift */
else if (w == L'n')
    n->term->gs = n->term->gc = n->term->g2; /* locking shift */
else if (w == L'o')
    n->term->gs = n->term->gc = n->term->g3; /* locking shift */
else if (w == L'N')
{
    n->term->gs = n->term->gc; /* non-locking shift */
    n->term->gc = n->term->g2;
}
else if (w == L'O')
{
    n->term->gs = n->term->gc; /* non-locking shift */
    n->term->gc = n->term->g3;
}
ENDHANDLER

static void setupevents(const std::unique_ptr<VtParser> &vp)
{
    vp->setControl(0x05, ack);
    vp->setControl(0x07, bell);
    vp->setControl(0x08, cub);
    vp->setControl(0x09, tab);
    vp->setControl(0x0a, pnl);
    vp->setControl(0x0b, pnl);
    vp->setControl(0x0c, pnl);
    vp->setControl(0x0d, cr);
    vp->setControl(0x0e, so);
    vp->setControl(0x0f, so);
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
    vp->setEscape(L'p', vis);
    vp->setEscape(L'=', numkp);
    vp->setEscape(L'>', numkp);
    vp->setPrint(print);
}

void sendarrow(const std::shared_ptr<NODE> &n, const char *k)
{
    char buf[100] = {0};
    snprintf(buf, sizeof(buf) - 1, "\033%s%s", n->term->pnm ? "O" : "[", k);
    n->term->safewrite(buf);
}

bool handlechar(int r, int k) /* Handle a single input character. */
{
    const char cmdstr[] = {(char)global::get_commandKey(), 0};
    static bool cmd = false;
    auto n = focused.lock();
#define KERR(i) (r == ERR && (i) == k)
#define KEY(i) (r == OK && (i) == k)
#define CODE(i) (r == KEY_CODE_YES && (i) == k)
#define INSCR (n->term->s->tos != n->term->s->off)
#define SB n->term->s->scrollbottom()
#define DO(s, t, a)                                                            \
    if (s == cmd && (t))                                                       \
    {                                                                          \
        a;                                                                     \
        cmd = false;                                                           \
        return true;                                                           \
    }

    DO(cmd, KERR(k), return false)
    DO(cmd, CODE(KEY_RESIZE), root->reshape(Rect(0, 0, LINES, COLS)); SB)
    DO(false, KEY(global::get_commandKey()), return cmd = true)
    DO(false, KEY(0), n->term->safewrite("\000", 1); SB)
    DO(false, KEY(L'\n'), n->term->safewrite("\n"); SB)
    DO(false, KEY(L'\r'), n->term->safewrite(n->term->lnm ? "\r\n" : "\r"); SB)
    DO(false, SCROLLUP && INSCR, n->term->s->scrollback(n->m_rect.h))
    DO(false, SCROLLDOWN && INSCR, n->term->s->scrollforward(n->m_rect.h))
    DO(false, RECENTER && INSCR, n->term->s->scrollbottom())
    DO(false, CODE(KEY_ENTER), n->term->safewrite(n->term->lnm ? "\r\n" : "\r"); SB)
    DO(false, CODE(KEY_UP), sendarrow(n, "A"); SB);
    DO(false, CODE(KEY_DOWN), sendarrow(n, "B"); SB);
    DO(false, CODE(KEY_RIGHT), sendarrow(n, "C"); SB);
    DO(false, CODE(KEY_LEFT), sendarrow(n, "D"); SB);
    DO(false, CODE(KEY_HOME), n->term->safewrite("\033[1~"); SB)
    DO(false, CODE(KEY_END), n->term->safewrite("\033[4~"); SB)
    DO(false, CODE(KEY_PPAGE), n->term->safewrite("\033[5~"); SB)
    DO(false, CODE(KEY_NPAGE), n->term->safewrite("\033[6~"); SB)
    DO(false, CODE(KEY_BACKSPACE), n->term->safewrite("\177"); SB)
    DO(false, CODE(KEY_DC), n->term->safewrite("\033[3~"); SB)
    DO(false, CODE(KEY_IC), n->term->safewrite("\033[2~"); SB)
    DO(false, CODE(KEY_BTAB), n->term->safewrite("\033[Z"); SB)
    DO(false, CODE(KEY_F(1)), n->term->safewrite("\033OP"); SB)
    DO(false, CODE(KEY_F(2)), n->term->safewrite("\033OQ"); SB)
    DO(false, CODE(KEY_F(3)), n->term->safewrite("\033OR"); SB)
    DO(false, CODE(KEY_F(4)), n->term->safewrite("\033OS"); SB)
    DO(false, CODE(KEY_F(5)), n->term->safewrite("\033[15~"); SB)
    DO(false, CODE(KEY_F(6)), n->term->safewrite("\033[17~"); SB)
    DO(false, CODE(KEY_F(7)), n->term->safewrite("\033[18~"); SB)
    DO(false, CODE(KEY_F(8)), n->term->safewrite("\033[19~"); SB)
    DO(false, CODE(KEY_F(9)), n->term->safewrite("\033[20~"); SB)
    DO(false, CODE(KEY_F(10)), n->term->safewrite("\033[21~"); SB)
    DO(false, CODE(KEY_F(11)), n->term->safewrite("\033[23~"); SB)
    DO(false, CODE(KEY_F(12)), n->term->safewrite("\033[24~"); SB)
    DO(true, MOVE_UP, focus(root->findnode(n->m_rect.above())))
    DO(true, MOVE_DOWN, focus(root->findnode(n->m_rect.below())))
    DO(true, MOVE_LEFT, focus(root->findnode(n->m_rect.left())))
    DO(true, MOVE_RIGHT, focus(root->findnode(n->m_rect.right())))
    DO(true, MOVE_OTHER, focus(lastfocused.lock()))
    DO(true, HSPLIT, split(n, HORIZONTAL))
    DO(true, VSPLIT, split(n, VERTICAL))
    DO(true, DELETE_NODE, deletenode(n))
    DO(true, REDRAW, touchwin(stdscr); root->draw(); redrawwin(stdscr))
    DO(true, SCROLLUP, n->term->s->scrollback(n->m_rect.h))
    DO(true, SCROLLDOWN, n->term->s->scrollforward(n->m_rect.h))
    DO(true, RECENTER, n->term->s->scrollbottom())
    DO(true, KEY(global::get_commandKey()), n->term->safewrite(cmdstr, 1));
    char c[MB_LEN_MAX + 1] = {0};
    if (wctomb(c, k) > 0)
    {
        n->term->s->scrollbottom();
        n->term->safewrite(c);
    }
    return cmd = false, true;
}

void vp_initialize(const std::unique_ptr<VtParser> &vp, void *p)
{
    setupevents(vp);
    ris(p, L'c', 0, 0, NULL, NULL);
}
