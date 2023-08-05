#pragma once
extern "C" {
#include "vtparser.h"
}
#include "config.h"
#include <curses.h>
#include <sys/select.h>
#include <vector>

extern fd_set fds;
extern int commandkey;
#define CTL(x) ((x)&0x1f)
extern const char *term;

void quit(int rc, const char *m); /* Shut down MTM. */

struct SCRN {
  int sy, sx, vis, tos, off;
  short fg, bg, sfg, sbg, sp;
  bool insert, oxenl, xenl, saved;
  attr_t sattr;
  WINDOW *win;
};

/*** DATA TYPES */

struct NODE {
  int y, x, h, w, pt;
  bool pnm, decom, am, lnm;
  std::vector<bool> tabs;
  wchar_t repc;
  // NODE *p, *c1, *c2;
  SCRN pri, alt, *s;
  wchar_t *g0, *g1, *g2, *g3, *gc, *gs, *sgc, *sgs;
  VTPARSER vp;

  NODE(int y, int x, int h, int w);
  ~NODE();
};

NODE *newview(NODE *p, int y, int x, int h, int w); /* Open a new view. */
extern NODE *root;
void deletenode(NODE *n); /* Delete a node. */

bool handlechar(int r, int k); /* Handle a single input character. */

extern int nfds; /* stdin */
void draw(NODE *n);
