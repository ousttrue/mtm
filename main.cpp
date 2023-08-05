#include "config.h"
#include "curses_screen.h"
#include "curses_term.h"
#include "node.h"
#include "posix_process.h"
#include "posix_selector.h"
#include "vtparser.h"
#include <curses.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <vterm.h>

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
  DO(false, input.KEY(0), n->Process->Write("\000", 1); n->s->scrollbottom())
  DO(false, input.KEY(L'\n'), n->Process->WriteString("\n");
     n->s->scrollbottom())
  DO(false, input.KEY(L'\r'), n->Process->WriteString(n->lnm ? "\r\n" : "\r");
     n->s->scrollbottom())
  DO(false, SCROLLUP && n->s->INSCR(), n->s->scrollback(n->Size.Rows / 2))
  DO(false, SCROLLDOWN && n->s->INSCR(), n->s->scrollforward(n->Size.Rows / 2))
  DO(false, RECENTER && n->s->INSCR(), n->s->scrollbottom())
  DO(false, input.CODE(KEY_ENTER),
     n->Process->WriteString(n->lnm ? "\r\n" : "\r");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_UP), n->sendarrow("A"); n->s->scrollbottom());
  DO(false, input.CODE(KEY_DOWN), n->sendarrow("B"); n->s->scrollbottom());
  DO(false, input.CODE(KEY_RIGHT), n->sendarrow("C"); n->s->scrollbottom());
  DO(false, input.CODE(KEY_LEFT), n->sendarrow("D"); n->s->scrollbottom());
  DO(false, input.CODE(KEY_HOME), n->Process->WriteString("\033[1~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_END), n->Process->WriteString("\033[4~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_PPAGE), n->Process->WriteString("\033[5~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_NPAGE), n->Process->WriteString("\033[6~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_BACKSPACE), n->Process->WriteString("\177");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_DC), n->Process->WriteString("\033[3~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_IC), n->Process->WriteString("\033[2~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_BTAB), n->Process->WriteString("\033[Z");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(1)), n->Process->WriteString("\033OP");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(2)), n->Process->WriteString("\033OQ");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(3)), n->Process->WriteString("\033OR");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(4)), n->Process->WriteString("\033OS");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(5)), n->Process->WriteString("\033[15~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(6)), n->Process->WriteString("\033[17~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(7)), n->Process->WriteString("\033[18~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(8)), n->Process->WriteString("\033[19~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(9)), n->Process->WriteString("\033[20~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(10)), n->Process->WriteString("\033[21~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(11)), n->Process->WriteString("\033[23~");
     n->s->scrollbottom())
  DO(false, input.CODE(KEY_F(12)), n->Process->WriteString("\033[24~");
     n->s->scrollbottom())
  // DO(true, DELETE_NODE, g_root = {})
  DO(true, REDRAW, touchwin(stdscr); n->s->draw(n->Pos, n->Size);
     redrawwin(stdscr))
  DO(true, SCROLLUP, n->s->scrollback(n->Size.Rows / 2))
  DO(true, SCROLLDOWN, n->s->scrollforward(n->Size.Rows / 2))
  DO(true, RECENTER, n->s->scrollbottom())
  DO(true, input.KEY(commandkey), n->Process->Write(cmdstr, 1));

  char c[MB_LEN_MAX + 1] = {0};
  if (wctomb(c, input.Char) > 0) {
    n->s->scrollbottom();
    n->Process->WriteString(c);
  }
  cmd = false;
  return true;
}

static void run(const std::shared_ptr<NODE> &node) {
  node->s->draw(node->Pos, node->Size);
  while (true) {

    Selector::Instance().Select();

    while (true) {
      auto input = node->s->getchar();
      if (!handlechar(node, input)) {
        break;
      }
    }

    if (auto span = Selector::Instance().Read(node->Process->FD())) {
      if (span->size()) {
#if USE_VTERM
        vterm_input_write(node->m_vterm, span->data(), span->size());
#else
        vtwrite(node->vp.get(), span->data(), span->size());
#endif
      }
    } else {
      // error exit
      break;
    }

#if USE_VTERM
    vterm_screen_flush_damage(node->m_vtscreen);
    int rows, cols;
    vterm_get_size(node->m_vterm, &rows, &cols);
    for (int y = 0; y < rows; ++y) {
      for (int x = 0; x < cols; ++x) {
        VTermScreenCell cell;
        vterm_screen_get_cell(node->m_vtscreen, {y, x}, &cell);

        wchar_t ch = cell.chars[0];
        if (ch>0 && ch < 128) {
          if (ch != ' ') {
            auto a = 0;
          }
          node->s->WriteCell({y + node->s->tos, x}, ch, cell.fg.indexed.idx,
                             cell.bg.indexed.idx);
        }
      }
    }
#else
#endif
    node->s->draw(node->Pos, node->Size);
    node->s->fixcursor(node->Size);
    node->s->draw(node->Pos, node->Size);
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

  auto node = std::make_shared<NODE>(
      POS{
          .Y = 2,
          .X = 2,
      },
      SIZE{
          .Rows = (uint16_t)(LINES - 4),
          .Cols = (uint16_t)(COLS - 4),
      });
  if (!node) {
    std::cout << "could not open root window" << std::endl;
    return EXIT_FAILURE;
  }

  node->Process = PosixProcess::Fork(node->Size, term);
  if (!node->Process) {
    return 0;
  }

  run(node);

  return EXIT_SUCCESS;
}
