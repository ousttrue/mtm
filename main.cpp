#include <algorithm>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#define USAGE "usage: mtm [-T NAME] [-t NAME] [-c KEY]\n"

extern "C" {
#include "mtm.h"
#include "pair.h"
}

static char iobuf[BUFSIZ];

static bool getinput(NODE *n,
                     fd_set *f) /* Recursively check all ptty's for input. */
{
  if (n && n->c1 && !getinput(n->c1, f))
    return false;

  if (n && n->c2 && !getinput(n->c2, f))
    return false;

  if (n && n->t == VIEW && n->pt > 0 && FD_ISSET(n->pt, f)) {
    ssize_t r = read(n->pt, iobuf, sizeof(iobuf));
    if (r > 0)
      vtwrite(&n->vp, iobuf, r);
    if (r <= 0 && errno != EINTR && errno != EWOULDBLOCK)
      return deletenode(n), false;
  }

  return true;
}

static void fixcursor(void) /* Move the terminal cursor to the active view. */
{
  if (focused) {
    int y, x;
    curs_set(focused->s->off != focused->s->tos ? 0 : focused->s->vis);
    getyx(focused->s->win, y, x);
    y = std::min(std::max(y, focused->s->tos),
                 focused->s->tos + focused->h - 1);
    wmove(focused->s->win, y, x);
  }
}

static void run(void) /* Run MTM. */
{
  while (root) {
    wint_t w = 0;
    fd_set sfds = fds;
    if (select(nfds + 1, &sfds, NULL, NULL, NULL) < 0)
      FD_ZERO(&sfds);

    int r = wget_wch(focused->s->win, &w);
    while (handlechar(r, w))
      r = wget_wch(focused->s->win, &w);
    getinput(root, &sfds);

    draw(root);
    doupdate();
    fixcursor();
    draw(focused);
    doupdate();
  }
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
