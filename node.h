#pragma once
#include "screen.h"
#include <memory>
#include <stdint.h>
#include <vector>

#define USE_VTERM 1

struct VTerm;
struct VTermScreen;

namespace term_screen {

class Process;

struct NODE {
  POS Pos;
  SIZE Size;

  std::shared_ptr<Process> Process;

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

#if USE_VTERM
  VTerm *m_vterm;
  VTermScreen *m_vtscreen = nullptr;
#else
  std::shared_ptr<struct VTPARSER> vp;
#endif

  NODE(const POS &pos, const SIZE &size);
  NODE(const NODE &) = delete;
  NODE &operator=(const NODE &) = delete;
  ~NODE();

  // pty
  void sendarrow(const char *k);
  // curses
  void reshape(const POS &pos, const SIZE &size);
  void reshapeview(int d);
};

} // namespace term_screen
