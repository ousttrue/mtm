#include "config.h"
#include "curses_screen.h"
#include "curses_term.h"
#include "mtm.h"
#include "node.h"
#include "posix_process.h"
#include "posix_selector.h"
#include <curses.h>
#include <iostream>
#include <signal.h>
#include <string.h>
extern "C" {
#include "pair.h"
#include "vtparser.h"
}

#define USAGE "usage: mtm [-T NAME] [-t NAME] [-c KEY]\n"
#define CTL(x) ((x)&0x1f)

/*** GLOBALS AND PROTOTYPES */
int commandkey = CTL(COMMAND_KEY);

/* The scrollback keys. */
#define SCROLLUP input.CODE(KEY_PPAGE)
#define SCROLLDOWN input.CODE(KEY_NPAGE)
#define RECENTER input.CODE(KEY_END)

/* The split terminal keys. */
#define HSPLIT input.KEY(L'h')
#define VSPLIT input.KEY(L'v')

/* The delete terminal key. */
#define DELETE_NODE input.KEY(L'w')

/* The force redraw key. */
#define REDRAW input.KEY(L'l')

/* Handle a single input character. */
static bool handlechar(const std::shared_ptr<NODE> &n,
                       const Input &input /*int r, int k*/) {
  const char cmdstr[] = {(char)commandkey, 0};
  static bool cmd = false;

#define DO(s, t, a)                                                            \
  if (s == cmd && (t)) {                                                       \
    a;                                                                         \
    cmd = false;                                                               \
    return true;                                                               \
  }

  DO(cmd, input.KERR(), return false)
  DO(cmd, input.CODE(KEY_RESIZE),
     n->reshape({0, 0}, {(uint16_t)LINES, (uint16_t)COLS});
     n->s->scrollbottom())
  DO(false, input.KEY(commandkey), return cmd = true)
  DO(false, input.KEY(0), n->SENDN("\000", 1); n->s->scrollbottom())
  DO(false, input.KEY(L'\n'), n->SEND("\n"); n->s->scrollbottom())
  DO(false, input.KEY(L'\r'), n->SEND(n->lnm ? "\r\n" : "\r");
     n->s->scrollbottom())
  DO(false, SCROLLUP && n->s->INSCR(), n->s->scrollback(n->Size.Rows / 2))
  DO(false, SCROLLDOWN && n->s->INSCR(), n->s->scrollforward(n->Size.Rows / 2))
  DO(false, RECENTER && n->s->INSCR(), n->s->scrollbottom())
  DO(false, input.CODE(KEY_ENTER), n->SEND(n->lnm ? "\r\n" : "\r");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_UP), n->sendarrow("A"); n->s->scrollbottom());
  DO(false, input.CODE(KEY_DOWN), n->sendarrow("B"); n->s->scrollbottom());
  DO(false, input.CODE(KEY_RIGHT), n->sendarrow("C"); n->s->scrollbottom());
  DO(false, input.CODE(KEY_LEFT), n->sendarrow("D"); n->s->scrollbottom());
  DO(false, input.CODE(KEY_HOME), n->SEND("\033[1~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_END), n->SEND("\033[4~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_PPAGE), n->SEND("\033[5~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_NPAGE), n->SEND("\033[6~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_BACKSPACE), n->SEND("\177"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_DC), n->SEND("\033[3~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_IC), n->SEND("\033[2~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_BTAB), n->SEND("\033[Z"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(1)), n->SEND("\033OP"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(2)), n->SEND("\033OQ"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(3)), n->SEND("\033OR"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(4)), n->SEND("\033OS"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(5)), n->SEND("\033[15~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(6)), n->SEND("\033[17~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(7)), n->SEND("\033[18~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(8)), n->SEND("\033[19~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(9)), n->SEND("\033[20~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(10)), n->SEND("\033[21~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(11)), n->SEND("\033[23~"); n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(12)), n->SEND("\033[24~"); n->s->scrollbottom())
  // DO(true, DELETE_NODE, g_root = {})
  DO(true, REDRAW, touchwin(stdscr); n->s->draw(n->Pos, n->Size);
     redrawwin(stdscr))
  DO(true, SCROLLUP, n->s->scrollback(n->Size.Rows / 2))
  DO(true, SCROLLDOWN, n->s->scrollforward(n->Size.Rows / 2))
  DO(true, RECENTER, n->s->scrollbottom())
  DO(true, input.KEY(commandkey), n->SENDN(cmdstr, 1));

  char c[MB_LEN_MAX + 1] = {0};
  if (wctomb(c, input.Char) > 0) {
    n->s->scrollbottom();
    n->SEND(c);
  }
  cmd = false;
  return true;
}

static void run(const std::shared_ptr<NODE> &g_root) /* Run MTM. */
{
  while (true) {

    Selector::Instance().Select();

    while (true) {
      auto input = g_root->s->getchar();
      if (!handlechar(g_root, input)) {
        break;
      }
    }

    if (auto span = Selector::Instance().Read(g_root->Process->FD())) {
      if (span->size()) {
        vtwrite(g_root->vp.get(), span->data(), span->size());
      }
    } else {
      // error
      break;
    }

    g_root->s->draw(g_root->Pos, g_root->Size);
    g_root->s->fixcursor(g_root->Size);
    g_root->s->draw(g_root->Pos, g_root->Size);
  }
}

static std::shared_ptr<NODE> newview(const POS &pos, const SIZE &size,
                                     const char *term = nullptr) {
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

  if (auto process = PosixProcess::Fork(size, term)) {
    if (n->Process) {
      Selector::Instance().Register(n->Process->FD());
    }
    return n;
  } else {
    return {};
  }
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  /* automatically reap children */
  signal(SIGCHLD, SIG_IGN);

  int c = 0;
  const char *term = nullptr;
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

  auto g_root = newview(
      POS{
          .Y = 2,
          .X = 2,
      },
      SIZE{
          .Rows = (uint16_t)(LINES - 4),
          .Cols = (uint16_t)(COLS - 4),
      },
      term);
  if (!g_root) {
    std::cout << "could not open root window" << std::endl;
    return EXIT_FAILURE;
  }
  g_root->s->draw(g_root->Pos, g_root->Size);
  run(g_root);

  return EXIT_SUCCESS;
}
