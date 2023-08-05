#pragma once
extern "C" {
#include "vtparser.h"
}
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
struct POS {
  int Y;
  int X;

  bool operator==(const POS &rhs) const { return Y == rhs.Y && X == rhs.X; }
};
struct SIZE {
  uint16_t Rows;
  uint16_t Cols;

  bool operator==(const SIZE &rhs) const {
    return Rows == rhs.Rows && Cols == rhs.Cols;
  }
  SIZE Max(const SIZE &rhs) const {
    return {
        std::max(Rows, rhs.Rows),
        std::max(Cols, rhs.Cols),
    };
  }
};

struct NODE {
  POS Pos;
  SIZE Size;

  // pty
  int pt = -1;
  bool pnm, decom, am, lnm;
  std::vector<bool> tabs;
  wchar_t repc;

  SCRN pri, alt, *s;
  wchar_t *g0, *g1, *g2, *g3, *gc, *gs, *sgc, *sgs;
  VTPARSER vp;

  NODE(const POS &pos, const SIZE &size);
  ~NODE();
  void draw() const;
  void reshape(const POS &pos, const SIZE &size);
};

// Open a new view
NODE *newview(const POS &pos, const SIZE &size);
extern NODE *root;
// Handle a single input character.
bool handlechar(int r, int k);
// stdin
extern int nfds;
