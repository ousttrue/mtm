#pragma once
#include <memory>
#include <stdint.h>
#include <vector>
#include "curses_screen.h"

struct VTPARSER;

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
  void reshape(const POS &pos, const SIZE &size);
  void reshapeview(int d);
};
