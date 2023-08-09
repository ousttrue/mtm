#pragma once
#include <stdint.h>

namespace term_screen {

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

struct Input {
  int Error;
  uint32_t Char;

  bool KERR() const;
  bool KEY(uint32_t i) const;
  bool CODE(uint32_t i) const;
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

  SCRN(const SIZE &size);
  ~SCRN();

  void scrollforward(int n);
  void scrollback(int n);
  void scrollbottom();
  void draw(const POS &pos, const SIZE &size) const;
  void fixcursor(const SIZE &size);
  Input getchar();
  bool INSCR() const { return tos != off; }

  POS GetPos() const;
  void Resize(const SIZE &size);
  void SetScrollRegion(int top, int bottom);
  void MoveCursor(const POS &pos);
  void Scroll(int d);
  void Update();
  void WriteCell(const POS &pos, wchar_t ch, int fg, int bg);
};

} // namespace term_screen
