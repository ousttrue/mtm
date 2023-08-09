#include "screen.h"

namespace term_screen {

SCRN::SCRN(const SIZE &size) {}
SCRN::~SCRN() {}

void SCRN::scrollforward(int n) {}
void SCRN::scrollback(int n) {}
void SCRN::scrollbottom() {}
void SCRN::draw(const POS &pos, const SIZE &size) const {}
void SCRN::fixcursor(const SIZE &size) {}
Input SCRN::getchar() { return {}; }

POS SCRN::GetPos() const { return {}; }
void SCRN::Resize(const SIZE &size) {}
void SCRN::SetScrollRegion(int top, int bottom) {}
void SCRN::MoveCursor(const POS &pos) {}
void SCRN::Scroll(int d) {}
void SCRN::Update() {}
void SCRN::WriteCell(const POS &pos, wchar_t ch, int fg, int bg) {}

} // namespace term_screen
