#pragma once
#include <stdint.h>
// #include <curses.h>

struct SCRN {
  int sy, sx;
  int vis;
  int tos;
  int off;
  short fg, bg;
  short sfg, sbg;
  short sp;
  bool insert;
  bool oxenl;
  bool xenl;
  bool saved;
  uint32_t sattr;
  struct _win_st *win;
};
