extern "C" {
#include "vtparser.h"
}
#include "config.h"
#include "curses_screen.h"
#include "node.h"
#include "posix_selector.h"
#include <algorithm>
#include <curses.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

SIZE SIZE::Max(const SIZE &rhs) const {
  return {
      std::max(Rows, rhs.Rows),
      std::max(Cols, rhs.Cols),
  };
}

NODE::NODE(const POS &pos, const SIZE &size)
    : Pos(pos), Size(size), pri(new SCRN), alt(new SCRN), vp(new VTPARSER) {
  this->tabs.resize(Size.Cols, 0);
}

NODE::~NODE() /* Free a node. */
{
  if (this->pri->win)
    delwin(this->pri->win);
  if (this->alt->win)
    delwin(this->alt->win);
  if (this->pt >= 0) {
    close(this->pt);
    Selector::Instance().Unregister(this->pt);
  }
}

void NODE::safewrite(const char *b, size_t n) /* Write, checking for errors. */
{
  size_t w = 0;
  while (w < n) {
    ssize_t s = write(this->pt, b + w, n - w);
    if (s < 0 && errno != EINTR)
      return;
    else if (s < 0)
      s = 0;
    w += (size_t)s;
  }
}

void NODE::reshape(const POS &pos, const SIZE &size) {
  if (this->Pos == pos && this->Size == size) {
    return;
  }

  int d = this->Size.Rows - size.Rows;
  this->Pos = pos;
  this->Size = size.Max({1, 1});
  this->tabs.resize(Size.Cols);
  this->reshapeview(d);
  this->s->draw(Pos, Size);
}

void NODE::SEND(const char *s) { SENDN(s, strlen(s)); }

void NODE::sendarrow(const char *k) {
  char buf[100] = {0};
  snprintf(buf, sizeof(buf) - 1, "\033%s%s", this->pnm ? "O" : "[", k);
  SEND(buf);
}

void NODE::reshapeview(int d) {

  int oy, ox;
  getyx(this->s->win, oy, ox);
  wresize(this->pri->win, std::max(this->Size.Rows, (uint16_t)SCROLLBACK),
          std::max(this->Size.Cols, (uint16_t)2));
  wresize(this->alt->win, std::max(this->Size.Rows, (uint16_t)2),
          std::max(this->Size.Cols, (uint16_t)2));
  this->pri->tos = this->pri->off =
      std::max(0, (uint16_t)SCROLLBACK - this->Size.Rows);
  this->alt->tos = this->alt->off = 0;
  wsetscrreg(this->pri->win, 0,
             std::max((uint16_t)SCROLLBACK, this->Size.Rows) - 1);
  wsetscrreg(this->alt->win, 0, this->Size.Rows - 1);
  if (d > 0) { /* make sure the new top line syncs up after reshape */
    wmove(this->s->win, oy + d, ox);
    wscrl(this->s->win, -d);
  }
  doupdate();
  refresh();

  struct winsize ws = {
      .ws_row = this->Size.Rows,
      .ws_col = this->Size.Cols,
  };
  ioctl(this->pt, TIOCSWINSZ, &ws);
}


