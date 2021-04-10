#include "config.h"
#include "node.h"
#include "minmax.h"
#include <cstdlib>

NODE *root = nullptr;
NODE *focused = nullptr;
NODE *lastfocused = nullptr;

static bool *newtabs(int w, int ow,
                     bool *oldtabs) /* Initialize default tabstops. */
{
    bool *tabs = (bool *)calloc(w, sizeof(bool));
    if (!tabs)
        return NULL;
    for (int i = 0; i < w; i++) /* keep old overlapping tabs */
        tabs[i] = i < ow ? oldtabs[i] : (i % 8 == 0);
    return tabs;
}

NODE *NODE::newnode(Node t, NODE *p, int y, int x, int h,
                    int w) /* Create a new node. */
{
    NODE *n = (NODE *)calloc(1, sizeof(NODE));
    bool *tabs = newtabs(w, 0, NULL);
    if (!n || h < 2 || w < 2 || !tabs)
        return free(n), free(tabs), nullptr;

    n->t = t;
    n->pt = -1;
    n->p = p;
    n->y = y;
    n->x = x;
    n->h = h;
    n->w = w;
    n->tabs = tabs;
    n->ntabs = w;
    n->pri = std::make_shared<SCRN>();
    n->alt = std::make_shared<SCRN>();

    return n;
}

void NODE::reshape(int y, int x, int h, int w) /* Reshape a node. */
{
    if (this->y == y && this->x == x && this->h == h && this->w == w &&
        this->t == VIEW)
        return;

    int d = this->h - h;
    int ow = this->w;
    this->y = y;
    this->x = x;
    this->h = MAX(h, 1);
    this->w = MAX(w, 1);

    if (this->t == VIEW)
        reshapeview(d, ow);
    else
        reshapechildren();
    this->draw();
}

void NODE::reshapechildren() /* Reshape all children of a view. */
{
    if (this->t == HORIZONTAL)
    {
        int i = this->w % 2 ? 0 : 1;
        this->c1->reshape(this->y, this->x, this->h, this->w / 2);
        this->c2->reshape(this->y, this->x + this->w / 2 + 1, this->h,
                          this->w / 2 - i);
    }
    else if (this->t == VERTICAL)
    {
        int i = this->h % 2 ? 0 : 1;
        this->c1->reshape(this->y, this->x, this->h / 2, this->w);
        this->c2->reshape(this->y + this->h / 2 + 1, this->x, this->h / 2 - i,
                          this->w);
    }
}

void NODE::reshapeview(int d, int ow) /* Reshape a view. */
{
    int oy, ox;
    bool *tabs = newtabs(this->w, ow, this->tabs);
    struct winsize ws = {.ws_row = (unsigned short)this->h,
                         .ws_col = (unsigned short)this->w};

    if (tabs)
    {
        free(this->tabs);
        this->tabs = tabs;
        this->ntabs = this->w;
    }

    getyx(this->s->win, oy, ox);
    wresize(this->pri->win, MAX(this->h, SCROLLBACK), MAX(this->w, 2));
    wresize(this->alt->win, MAX(this->h, 2), MAX(this->w, 2));
    this->pri->tos = this->pri->off = MAX(0, SCROLLBACK - this->h);
    this->alt->tos = this->alt->off = 0;
    wsetscrreg(this->pri->win, 0, MAX(SCROLLBACK, this->h) - 1);
    wsetscrreg(this->alt->win, 0, this->h - 1);
    if (d > 0)
    { /* make sure the new top line syncs up after reshape */
        wmove(this->s->win, oy + d, ox);
        wscrl(this->s->win, -d);
    }
    doupdate();
    refresh();
    ioctl(this->pt, TIOCSWINSZ, &ws);
}

void NODE::draw() const /* Draw a node. */
{
    if (this->t == VIEW)
        pnoutrefresh(this->s->win, this->s->off, 0, this->y, this->x,
                     this->y + this->h - 1, this->x + this->w - 1);
    else
        drawchildren();
}

void NODE::drawchildren() const /* Draw all children of n. */
{
    this->c1->draw();
    if (this->t == HORIZONTAL)
        mvvline(this->y, this->x + this->w / 2, ACS_VLINE, this->h);
    else
        mvhline(this->y + this->h / 2, this->x, ACS_HLINE, this->w);
    wnoutrefresh(stdscr);
    this->c2->draw();
}
