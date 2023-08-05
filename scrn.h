#pragma once
#include <curses.h>

struct SCRN {
  int sy, sx, vis, tos, off;
  short fg, bg, sfg, sbg, sp;
  bool insert, oxenl, xenl, saved;
  attr_t sattr;
  WINDOW *win;
};
