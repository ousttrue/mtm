#pragma once
#include <memory>
#include <stdint.h>
#include <vector>

struct SCRN;
struct VTPARSER;

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

struct NODE {
  POS Pos;
  SIZE Size;

  // pty
  int pt = -1;

  bool pnm;
  bool decom;
  bool am;
  bool lnm;
  std::vector<bool> tabs;
  wchar_t repc;
  std::shared_ptr<SCRN> pri;
  std::shared_ptr<SCRN> alt;
  std::shared_ptr<SCRN> s;
  wchar_t *g0, *g1, *g2, *g3, *gc, *gs, *sgc, *sgs;
  std::shared_ptr<VTPARSER> vp;

  NODE(const POS &pos, const SIZE &size);
  NODE(const NODE &) = delete;
  NODE &operator=(const NODE &) = delete;
  ~NODE();

  // pty
  void safewrite(const char *b, size_t n);
  void SENDN(const char *s, size_t c) { safewrite(s, c); }
  void SEND(const char *s);
  void sendarrow(const char *k);
  // curses
  void draw() const;
  void reshape(const POS &pos, const SIZE &size);
  void reshapeview(int d);
  /* Move the terminal cursor to the active view. */
  void fixcursor(void);
  // scrn
  void scrollbottom();
  void scrollback();
  void scrollforward();
};
