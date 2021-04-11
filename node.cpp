#include "config.h"
#include "node.h"
#include "minmax.h"
#include "selector.h"
#include "scrn.h"
#include <cstdlib>

std::shared_ptr<NODE> root;
std::weak_ptr<NODE> focused;
std::weak_ptr<NODE> lastfocused;

NODE::NODE(Node t, const std::shared_ptr<NODE> &p, int y, int x, int h, int w)
{
    this->t = t;
    this->pt = -1;
    this->p = p;
    this->y = y;
    this->x = x;
    this->h = h;
    this->w = w;
    for (int i = 0; i < w; i++)
    {
        /* keep old overlapping tabs */
        tabs.push_back(i % 8 == 0);
    }
    this->ntabs = w;
    this->pri = std::make_shared<SCRN>();
    this->alt = std::make_shared<SCRN>();
}

NODE::~NODE()
{
    if (auto l = lastfocused.lock())
        if (l.get() == this)
            lastfocused.reset();
    if (this->pri->win)
        delwin(this->pri->win);
    if (this->alt->win)
        delwin(this->alt->win);
    if (this->pt >= 0)
    {
        selector::close(this->pt);
    }
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
    struct winsize ws = {.ws_row = (unsigned short)this->h,
                         .ws_col = (unsigned short)this->w};

    {
        this->ntabs = this->w;
        auto oldtabs = tabs;
        this->tabs.clear();
        for (int i = 0; i < w; i++) /* keep old overlapping tabs */
            tabs.push_back(i < ow ? oldtabs[i] : (i % 8 == 0));
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

bool NODE::IN(int y, int x) const
{
    return (y >= this->y && y <= this->y + this->h && x >= this->x &&
            x <= this->x + this->w);
}

std::shared_ptr<NODE> NODE::findnode(int y, int x) /* Find the node enclosing y,x. */
{
    if (IN(y, x))
    {
        if (this->c1 && this->c1->IN(y, x))
            return this->c1->findnode(y, x);
        if (this->c2 && this->c2->IN(y, x))
            return this->c2->findnode(y, x);
        return shared_from_this();
    }
    return NULL;
}
