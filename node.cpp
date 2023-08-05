#include "node.h"
#include "scrn.h"
#include <algorithm>
#include <errno.h>
#include <string.h>
#include <unistd.h>

SIZE SIZE::Max(const SIZE &rhs) const {
  return {
      std::max(Rows, rhs.Rows),
      std::max(Cols, rhs.Cols),
  };
}

void NODE::scrollbottom() { s->off = s->tos; }

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
  this->draw();
}

void NODE::draw() const /* Draw a node. */
{
  pnoutrefresh(this->s->win, this->s->off, 0, this->Pos.Y, this->Pos.X,
               this->Pos.Y + this->Size.Rows - 1,
               this->Pos.X + this->Size.Rows - 1);
}

void NODE::scrollback() {
  this->s->off = std::max(0, this->s->off - this->Size.Rows / 2);
}

void NODE::scrollforward() {
  this->s->off = std::min(this->s->tos, this->s->off + this->Size.Rows / 2);
}

void NODE::SEND(const char *s) { SENDN(s, strlen(s)); }

void NODE::sendarrow(const char *k) {
  char buf[100] = {0};
  snprintf(buf, sizeof(buf) - 1, "\033%s%s", this->pnm ? "O" : "[", k);
  SEND(buf);
}
