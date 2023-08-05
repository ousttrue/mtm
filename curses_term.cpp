#include "curses_term.h"
#include <curses.h>

Term::Term() {}

Term::~Term() { endwin(); }

bool Term::Initialize() { return initscr(); }

void Term::RawMode() {

  raw();
  noecho();
  nonl();
  intrflush(stdscr, FALSE);
  start_color();
  use_default_colors();
}
