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

const int COLOR_MAX = 256;

short Term::AllocPair(int fg, int bg) {
  if (fg >= COLOR_MAX || bg >= COLOR_MAX) {
    return -1;
  }

  for (int i = 0; i < m_pairs.size(); ++i) {
    if (m_pairs[i].fg == fg && m_pairs[i].bg == bg) {
      return i + 1;
    }
  }

  m_pairs.push_back({(short)fg, (short)bg});
  if (init_pair(m_pairs.size(), fg, bg) != OK) {
    // fail
    m_pairs.pop_back();
    return -1;
  }

  return m_pairs.size();
}
