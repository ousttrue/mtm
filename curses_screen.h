#pragma once
#include <stdint.h>

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
  SIZE Max(const SIZE &rhs) const;
};

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

  void scrollforward(int n);
  void scrollback(int n);
  void scrollbottom();
  void draw(const POS &pos, const SIZE &size) const;
  void fixcursor(const SIZE &size);
};
