#include "config.h"
#include "curses_term.h"
#include "mtm.h"
#include "node.h"
#include "posix_selector.h"
#include "scrn.h"
#include <algorithm>
#include <curses.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <limits.h>
#include <locale.h>
#include <pty.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
extern "C" {
#include "pair.h"
#include "vtparser.h"
}

#define USAGE "usage: mtm [-T NAME] [-t NAME] [-c KEY]\n"
#define CTL(x) ((x)&0x1f)

std::shared_ptr<NODE> g_root;
/*** GLOBALS AND PROTOTYPES */
int commandkey = CTL(COMMAND_KEY);
const char *term = NULL;

static bool handlechar(int r, int k) /* Handle a single input character. */
{
  const char cmdstr[] = {(char)commandkey, 0};
  static bool cmd = false;
  auto n = g_root;
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
     g_root->reshape({0, 0}, {(uint16_t)LINES, (uint16_t)COLS});
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
  DO(true, DELETE_NODE, g_root = {})
  DO(true, REDRAW, touchwin(stdscr); g_root->draw(); redrawwin(stdscr))
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
  while (g_root) {
    wint_t w = 0;

    Selector::Instance().Select();

    int r = wget_wch(g_root->s->win, &w);
    while (handlechar(r, w))
      r = wget_wch(g_root->s->win, &w);
    if (!g_root) {
      return;
    }

    if (auto span = Selector::Instance().Read(g_root->pt)) {
      if (span->size()) {
        vtwrite(g_root->vp.get(), span->data(), span->size());
      }
    } else {
      // error
      break;
    }

    g_root->draw();
    doupdate();
    g_root->fixcursor();
    g_root->draw();
    doupdate();
  }
}

static const char *getterm(void) {
  const char *envterm = getenv("TERM");
  if (term)
    return term;
  if (envterm && COLORS >= 256 && !strstr(DEFAULT_TERMINAL, "-256color"))
    return DEFAULT_256_COLOR_TERMINAL;
  return DEFAULT_TERMINAL;
}

static const char *getshell(void) /* Get the user's preferred shell. */
{
  if (getenv("SHELL"))
    return getenv("SHELL");
  struct passwd *pwd = getpwuid(getuid());
  if (pwd)
    return pwd->pw_shell;
  return "/bin/sh";
}

static std::shared_ptr<NODE> newview(const POS &pos, const SIZE &size) {
  struct winsize ws = {
      .ws_row = size.Rows,
      .ws_col = size.Cols,
  };
  auto n = std::make_shared<NODE>(pos, size);
  n->pri->win = newpad(std::max(size.Rows, (uint16_t)SCROLLBACK), size.Cols);
  n->alt->win = newpad(size.Rows, size.Cols);
  if (!n->pri->win || !n->alt->win) {
    return NULL;
  }
  n->pri->tos = n->pri->off = std::max(0, SCROLLBACK - size.Rows);
  n->s = n->pri;

  nodelay(n->pri->win, TRUE);
  nodelay(n->alt->win, TRUE);
  scrollok(n->pri->win, TRUE);
  scrollok(n->alt->win, TRUE);
  keypad(n->pri->win, TRUE);
  keypad(n->alt->win, TRUE);

  setupevents(n.get());

  pid_t pid = forkpty(&n->pt, NULL, NULL, &ws);
  if (pid < 0) {
    perror("forkpty");
    return nullptr;
  } else if (pid == 0) {
    char buf[100] = {0};
    snprintf(buf, sizeof(buf) - 1, "%lu", (unsigned long)getppid());
    setsid();
    setenv("MTM", buf, 1);
    setenv("TERM", getterm(), 1);
    signal(SIGCHLD, SIG_DFL);
    execl(getshell(), getshell(), NULL);
    return NULL;
  }

  Selector::Instance().Register(n->pt);
  return n;
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  /* automatically reap children */
  signal(SIGCHLD, SIG_IGN);

  int c = 0;
  while ((c = getopt(argc, argv, "c:T:t:")) != -1) {
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
      std::cout << USAGE << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (!Term::Insance().Initialize()) {
    std::cout << "could not initialize terminal" << std::endl;
    return EXIT_FAILURE;
  }
  Term::Insance().RawMode();

  start_pairs();

  g_root = newview(
      POS{
          .Y = 2,
          .X = 2,
      },
      SIZE{
          .Rows = (uint16_t)(LINES - 4),
          .Cols = (uint16_t)(COLS - 4),
      });
  if (!g_root) {
    std::cout << "could not open root window" << std::endl;
    return EXIT_FAILURE;
  }
  g_root->draw();
  run();

  return EXIT_SUCCESS;
}
