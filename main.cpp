#include "config.h"
#include "mtm.h"
#include "node.h"
#include "scrn.h"
#include <algorithm>
#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "pair.h"
#include "vtparser.h"
}

#define USAGE "usage: mtm [-T NAME] [-t NAME] [-c KEY]\n"

NODE *root = nullptr;

/*** UTILITY FUNCTIONS */
static void quit(int rc, const char *m) /* Shut down MTM. */
{
  if (m)
    fprintf(stderr, "%s\n", m);
  if (root)
    delete (root);
  endwin();
  exit(rc);
}

static char iobuf[BUFSIZ];

static bool getinput(NODE *n,
                     fd_set *f) /* Recursively check all ptty's for input. */
{
  if (n && n->pt > 0 && FD_ISSET(n->pt, f)) {
    ssize_t r = read(n->pt, iobuf, sizeof(iobuf));
    if (r > 0)
      vtwrite(n->vp.get(), iobuf, r);
    if (r <= 0 && errno != EINTR && errno != EWOULDBLOCK) {
      delete n;
      return false;
    }
  }

  return true;
}

static void fixcursor(void) /* Move the terminal cursor to the active view. */
{
  int y, x;
  curs_set(root->s->off != root->s->tos ? 0 : root->s->vis);
  getyx(root->s->win, y, x);
  y = std::min(std::max(y, root->s->tos), root->s->tos + root->Size.Rows - 1);
  wmove(root->s->win, y, x);
}

static bool handlechar(int r, int k) /* Handle a single input character. */
{
  const char cmdstr[] = {(char)commandkey, 0};
  static bool cmd = false;
  NODE *n = root;
#define KERR(i) (r == ERR && (i) == k)
#define KEY(i) (r == OK && (i) == k)
#define CODE(i) (r == KEY_CODE_YES && (i) == k)
#define INSCR (n->s->tos != n->s->off)
#define SB n->scrollbottom()
#define DO(s, t, a)                                                            \
  if (s == cmd && (t)) {                                                       \
    a;                                                                         \
    cmd = false;                                                               \
    return true;                                                               \
  }

  DO(cmd, KERR(k), return false)
  DO(cmd, CODE(KEY_RESIZE),
     root->reshape({0, 0}, {(uint16_t)LINES, (uint16_t)COLS});
     SB)
  DO(false, KEY(commandkey), return cmd = true)
  DO(false, KEY(0), n->SENDN("\000", 1); SB)
  DO(false, KEY(L'\n'), n->SEND("\n"); SB)
  DO(false, KEY(L'\r'), n->SEND(n->lnm ? "\r\n" : "\r"); SB)
  DO(false, SCROLLUP && INSCR, n->scrollback())
  DO(false, SCROLLDOWN && INSCR, n->scrollforward())
  DO(false, RECENTER && INSCR, n->scrollbottom())
  DO(false, CODE(KEY_ENTER), n->SEND(n->lnm ? "\r\n" : "\r"); SB)
  DO(false, CODE(KEY_UP), n->sendarrow("A"); SB);
  DO(false, CODE(KEY_DOWN), n->sendarrow("B"); SB);
  DO(false, CODE(KEY_RIGHT), n->sendarrow("C"); SB);
  DO(false, CODE(KEY_LEFT), n->sendarrow("D"); SB);
  DO(false, CODE(KEY_HOME), n->SEND("\033[1~"); SB)
  DO(false, CODE(KEY_END), n->SEND("\033[4~"); SB)
  DO(false, CODE(KEY_PPAGE), n->SEND("\033[5~"); SB)
  DO(false, CODE(KEY_NPAGE), n->SEND("\033[6~"); SB)
  DO(false, CODE(KEY_BACKSPACE), n->SEND("\177"); SB)
  DO(false, CODE(KEY_DC), n->SEND("\033[3~"); SB)
  DO(false, CODE(KEY_IC), n->SEND("\033[2~"); SB)
  DO(false, CODE(KEY_BTAB), n->SEND("\033[Z"); SB)
  DO(false, CODE(KEY_F(1)), n->SEND("\033OP"); SB)
  DO(false, CODE(KEY_F(2)), n->SEND("\033OQ"); SB)
  DO(false, CODE(KEY_F(3)), n->SEND("\033OR"); SB)
  DO(false, CODE(KEY_F(4)), n->SEND("\033OS"); SB)
  DO(false, CODE(KEY_F(5)), n->SEND("\033[15~"); SB)
  DO(false, CODE(KEY_F(6)), n->SEND("\033[17~"); SB)
  DO(false, CODE(KEY_F(7)), n->SEND("\033[18~"); SB)
  DO(false, CODE(KEY_F(8)), n->SEND("\033[19~"); SB)
  DO(false, CODE(KEY_F(9)), n->SEND("\033[20~"); SB)
  DO(false, CODE(KEY_F(10)), n->SEND("\033[21~"); SB)
  DO(false, CODE(KEY_F(11)), n->SEND("\033[23~"); SB)
  DO(false, CODE(KEY_F(12)), n->SEND("\033[24~"); SB)
  DO(true, DELETE_NODE, delete n)
  DO(true, REDRAW, touchwin(stdscr); root->draw(); redrawwin(stdscr))
  DO(true, SCROLLUP, n->scrollback())
  DO(true, SCROLLDOWN, n->scrollforward())
  DO(true, RECENTER, n->scrollbottom())
  DO(true, KEY(commandkey), n->SENDN(cmdstr, 1));
  char c[MB_LEN_MAX + 1] = {0};
  if (wctomb(c, k) > 0) {
    n->scrollbottom();
    n->SEND(c);
  }
  return cmd = false, true;
}
static void run(void) /* Run MTM. */
{
  while (root) {
    wint_t w = 0;
    fd_set sfds = fds;
    if (select(nfds + 1, &sfds, NULL, NULL, NULL) < 0)
      FD_ZERO(&sfds);

    int r = wget_wch(root->s->win, &w);
    while (handlechar(r, w))
      r = wget_wch(root->s->win, &w);
    getinput(root, &sfds);

    root->draw();
    doupdate();
    fixcursor();
    root->draw();
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

  root = newview(
      POS{
          .Y = 2,
          .X = 2,
      },
      SIZE{
          .Rows = (uint16_t)(LINES - 4),
          .Cols = (uint16_t)(COLS - 4),
      });
  if (!root)
    quit(EXIT_FAILURE, "could not open root window");
  root->draw();
  run();

  quit(EXIT_SUCCESS, NULL);
  return EXIT_SUCCESS; /* not reached */
}
