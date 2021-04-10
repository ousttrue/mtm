#include "node.h"
#include "scrn.h"

void scrollbottom(NODE *n)
{
    n->s->off = n->s->tos;
}
