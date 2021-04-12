#include "vtscreen.h"
#include "scrn.h"
#include "minmax.h"
#include "selector.h"
#include "vthandler.h"
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "vt/vtparser.h"
#include "config.h"

#ifdef __cplusplus
}
#endif


VTScreen::VTScreen(int h, int w)
{
    for (int i = 0; i < w; i++)
    {
        /* keep old overlapping tabs */
        tabs.push_back(i % 8 == 0);
    }

    this->pri = std::make_shared<SCRN>();
    this->alt = std::make_shared<SCRN>();

    this->pri->win = newpad(MAX(h, SCROLLBACK), w);
    this->alt->win = newpad(h, w);
    if (!pri->win || !alt->win)
    {
        throw "new pad";
        // return nullptr;
    }

    pri->tos = pri->off = MAX(0, SCROLLBACK - h);
    this->s = this->pri;

    nodelay(pri->win, TRUE);
    nodelay(alt->win, TRUE);
    scrollok(pri->win, TRUE);
    scrollok(alt->win, TRUE);
    keypad(pri->win, TRUE);
    keypad(alt->win, TRUE);

    this->vp = std::make_unique<VTPARSER>();
}

VTScreen::~VTScreen()
{
    if (this->pri->win)
        delwin(this->pri->win);
    if (this->alt->win)
        delwin(this->alt->win);
    if (this->pt >= 0)
    {
        selector::close(this->pt);
    }
}

void VTScreen::reshapeview(int d, int ow, int lines, int cols) /* Reshape a view. */
{
    int oy, ox;
    struct winsize ws = {.ws_row = (unsigned short)lines,
                         .ws_col = (unsigned short)cols};

    {
        auto oldtabs = tabs;
        this->tabs.clear();
        for (int i = 0; i < cols; i++) /* keep old overlapping tabs */
            tabs.push_back(i < ow ? oldtabs[i] : (i % 8 == 0));
    }

    getyx(this->s->win, oy, ox);
    wresize(this->pri->win, MAX(lines, SCROLLBACK), MAX(cols, 2));
    wresize(this->alt->win, MAX(lines, 2), MAX(cols, 2));
    this->pri->tos = this->pri->off = MAX(0, SCROLLBACK - lines);
    this->alt->tos = this->alt->off = 0;
    wsetscrreg(this->pri->win, 0, MAX(SCROLLBACK, lines) - 1);
    wsetscrreg(this->alt->win, 0, lines - 1);
    if (d > 0)
    { /* make sure the new top line syncs up after reshape */
        wmove(this->s->win, oy + d, ox);
        wscrl(this->s->win, -d);
    }
    doupdate();
    refresh();
    ioctl(this->pt, TIOCSWINSZ, &ws);
}

void VTScreen::draw(int y, int x, int h, int w)
{
    pnoutrefresh(this->s->win, this->s->off, 0, y, x, y + h - 1, x + w - 1);
}

bool VTScreen::process()
{
    if (this->pt > 0 && selector::isSet(this->pt))
    {
        char g_iobuf[BUFSIZ];
        ssize_t r = read(this->pt, g_iobuf, sizeof(g_iobuf));
        if (r > 0)
        {
            vtwrite(this->vp.get(), g_iobuf, r);
        }
        if (r <= 0 && errno != EINTR && errno != EWOULDBLOCK)
        {
            return false;
        }
    }

    return true;
}

bool VTScreen::handleUserInput()
{
    wint_t w = 0;
    int r = wget_wch(this->s->win, &w);
    return handlechar(r, w);
}

void VTScreen::fixCursor(int h)
{
    this->s->fixcursor(h);
}

void VTScreen::reset(int h)
{
    this->gs = this->gc = this->g0 = CSET_US;
    this->g1 = CSET_GRAPH;
    this->g2 = CSET_US;
    this->g3 = CSET_GRAPH;
    this->decom = s->insert = s->oxenl = s->xenl = this->lnm = false;
    this->am = true;
    this->pnm = false;
    this->pri->vis = this->alt->vis = 1;
    this->s = this->pri;
    wsetscrreg(this->pri->win, 0, MAX(SCROLLBACK, h) - 1);
    wsetscrreg(this->alt->win, 0, h - 1);
    for (int i = 0; i < this->tabs.size(); i++)
        this->tabs[i] = (i % 8 == 0);
}

bool VTScreen::alternate_screen_buffer_mode(bool set)
{
    auto n = this;
    if (set && n->s != n->alt)
    {
        n->s = n->alt;
        return true;
        // CALL(cls);
    }
    else if (!set && n->s != n->pri)
    {
        n->s = n->pri;
    }
    return false;
}
