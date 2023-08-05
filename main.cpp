#include <curses.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#define USAGE "usage: mtm [-T NAME] [-t NAME] [-c KEY]\n"

extern "C" {
#include "mtm.h"
#include "pair.h"
}

int main(int argc, char **argv) {
  FD_SET(STDIN_FILENO, &fds);
  setlocale(LC_ALL, "");
  signal(SIGCHLD, SIG_IGN); /* automatically reap children */

  int c = 0;
  while ((c = getopt(argc, argv, "c:T:t:")) != -1)
    switch (c) {
    case 'c':
      commandkey = CTL(optarg[0]);
      break;
    case 'T':
      setenv("TERM", optarg, 1);
      break;
    case 't':
      term = optarg;
      break;
    default:
      quit(EXIT_FAILURE, USAGE);
      break;
    }

  if (!initscr())
    quit(EXIT_FAILURE, "could not initialize terminal");
  raw();
  noecho();
  nonl();
  intrflush(stdscr, FALSE);
  start_color();
  use_default_colors();
  start_pairs();

  root = newview(NULL, 0, 0, LINES, COLS);
  if (!root)
    quit(EXIT_FAILURE, "could not open root window");
  focus(root);
  draw(root);
  run();

  quit(EXIT_SUCCESS, NULL);
  return EXIT_SUCCESS; /* not reached */
}
