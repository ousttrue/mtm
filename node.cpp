#include "node.h"
#include "config.h"
#include "curses_screen.h"
#include "mtm.h"
#include "posix_process.h"
#include "vtparser.h"
#include <vterm.h>

SIZE SIZE::Max(const SIZE &rhs) const {
  return {
      std::max(Rows, rhs.Rows),
      std::max(Cols, rhs.Cols),
  };
}

NODE::NODE(const POS &pos, const SIZE &size)
    : Pos(pos), Size(size),
      pri(new SCRN({std::max(size.Rows, (uint16_t)SCROLLBACK), size.Cols})),
      alt(new SCRN(size)), vp(new VTPARSER),
      m_vterm(vterm_new(size.Rows, size.Cols)) {
  this->tabs.resize(Size.Cols, 0);

  if (this->pri->win && this->alt->win) {
    this->pri->tos = this->pri->off = std::max(0, SCROLLBACK - size.Rows);
    this->s = this->pri;
    setupevents(this);
  }
}

NODE::~NODE() { vterm_free(m_vterm); }

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

void NODE::sendarrow(const char *k) {
  char buf[100] = {0};
  snprintf(buf, sizeof(buf) - 1, "\033%s%s", this->pnm ? "O" : "[", k);
  Process->WriteString(buf);
}

void NODE::reshapeview(int d) {
  auto pos = this->s->GetPos();

  this->pri->Resize(
      {std::max(this->Size.Rows, (uint16_t)SCROLLBACK), this->Size.Cols});
  this->alt->Resize(Size);
  this->pri->tos = this->pri->off =
      std::max(0, (uint16_t)SCROLLBACK - this->Size.Rows);
  this->alt->tos = this->alt->off = 0;
  this->pri->SetScrollRegion(
      0, std::max((uint16_t)SCROLLBACK, this->Size.Rows) - 1);
  this->alt->SetScrollRegion(0, this->Size.Rows - 1);
  if (d > 0) {
    /* make sure the new top line syncs up after reshape */
    this->s->MoveCursor({pos.Y + d, Pos.X});
    this->s->Scroll(-d);
  }
  this->s->Update();

  this->Process->Resize(this->Size);
}
