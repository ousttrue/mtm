#pragma once
#include <memory>

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

  std::shared_ptr<SCRN> pri;
  std::shared_ptr<SCRN> alt;
  std::shared_ptr<SCRN> s;
  wchar_t *g0, *g1, *g2, *g3, *gc, *gs, *sgc, *sgs;
  VTPARSER vp;

  NODE(const POS &pos, const SIZE &size);
  ~NODE();
  void draw() const;
  void reshape(const POS &pos, const SIZE &size);
};
