#pragma once
#include "vtparser.h"
#include <curses.h>
#include <sys/select.h>

extern fd_set fds;
extern int commandkey;
#define CTL(x) ((x)&0x1f)
extern const char *term;

void quit(int rc, const char *m); /* Shut down MTM. */

typedef struct SCRN SCRN;
struct SCRN {
  int sy, sx, vis, tos, off;
  short fg, bg, sfg, sbg, sp;
  bool insert, oxenl, xenl, saved;
  attr_t sattr;
  WINDOW *win;
};

/*** DATA TYPES */
typedef enum { HORIZONTAL, VERTICAL, VIEW } Node;

typedef struct NODE NODE;
struct NODE {
  Node t;
  int y, x, h, w, pt, ntabs;
  bool *tabs, pnm, decom, am, lnm;
  wchar_t repc;
  NODE *p, *c1, *c2;
  SCRN pri, alt, *s;
  wchar_t *g0, *g1, *g2, *g3, *gc, *gs, *sgc, *sgs;
  VTPARSER vp;
};

NODE *newview(NODE *p, int y, int x, int h, int w); /* Open a new view. */
extern NODE *root;
extern NODE *focused;
void focus(NODE *n);      /* Focus a node. */
void deletenode(NODE *n); /* Delete a node. */

bool handlechar(int r, int k); /* Handle a single input character. */

extern int nfds; /* stdin */
void draw(NODE *n);
