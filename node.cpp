#include "node.h"
#include "minmax.h"
#include <cstdlib>

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
