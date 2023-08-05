#include "curses_screen.h"
#include <algorithm>
#include <curses.h>

bool Input::KERR() const { return Error == ERR /*&& (i) == Char*/; }
bool Input::KEY(uint32_t i) const { return Error == OK && i == Char; }
bool Input::CODE(uint32_t i) const {
  return Error == KEY_CODE_YES && i == Char;
}

void SCRN::scrollforward(int n) {
  off = std::min(tos, off + n /*this->Size.Rows / 2*/);
}

void SCRN::scrollback(int n) {
  off = std::max(0, off - n /*this->Size.Rows / 2*/);
}

void SCRN::scrollbottom() { off = tos; }

void SCRN::draw(const POS &pos, const SIZE &size) const /* Draw a node. */
{
  pnoutrefresh(win, off, 0, pos.Y, pos.X, pos.Y + size.Rows - 1,
               pos.X + size.Rows - 1);
  doupdate();
}

void SCRN::fixcursor(
    const SIZE &size) /* Move the terminal cursor to the active view. */
{
  int y, x;
  curs_set(this->off != this->tos ? 0 : this->vis);
  getyx(this->win, y, x);
  y = std::min(std::max(y, this->tos), this->tos + size.Rows - 1);
  wmove(this->win, y, x);
}

Input SCRN::getchar() {
  Input input{};
  input.Error = wget_wch(win, &input.Char);
  return input;
}
