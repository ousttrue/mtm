#include "term.h"
#include <Windows.h>

namespace term_screen {

struct TermImpl {

  HANDLE m_hStdin = nullptr;
  DWORD m_fdwSaveOldMode = 0;

  TermImpl() {
    // Get the standard input handle.
    m_hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (m_hStdin == INVALID_HANDLE_VALUE) {
      // ErrorExit("GetStdHandle");
      return;
    }

    // Save the current input mode, to be restored on exit.

    if (!GetConsoleMode(m_hStdin, &m_fdwSaveOldMode)) {
      // ErrorExit("GetConsoleMode");
      return;
    }
  }

  ~TermImpl() {
    // Restore input mode on exit.
    SetConsoleMode(m_hStdin, m_fdwSaveOldMode);
  }

  void RawMode() {
    // Enable the window and mouse input events.
    auto fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    if (!SetConsoleMode(m_hStdin, fdwMode)) {
      // ErrorExit("SetConsoleMode");
      return;
    }

    // freopen("CON", "r", stdin);
  }

  INPUT_RECORD m_irInBuf[128];
  void Read() {

    const DWORD timeout = 1000 / 15; // 15 FPS
    auto wait = WaitForSingleObject(m_hStdin, timeout);
    if (wait != WAIT_OBJECT_0) {
      return;
    }

    // Wait for the events.
    DWORD cNumRead;
    if (!ReadConsoleInputA(m_hStdin,          // input buffer handle
                           m_irInBuf,         // buffer to read into
                           sizeof(m_irInBuf), // size of read buffer
                           &cNumRead))        // number of records read
    {
      // ErrorExit("ReadConsoleInput");
      return;
    }

    // Dispatch the events to the appropriate handler.

    for (int i = 0; i < cNumRead; i++) {
      switch (m_irInBuf[i].EventType) {
      case KEY_EVENT: // keyboard input
        // KeyEventProc(m_irInBuf[i].Event.KeyEvent);
        break;

      case MOUSE_EVENT: // mouse input
        // MouseEventProc(m_irInBuf[i].Event.MouseEvent);
        break;

      case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
        // ResizeEventProc(m_irInBuf[i].Event.WindowBufferSizeEvent);
        break;

      case FOCUS_EVENT: // disregard focus events

      case MENU_EVENT: // disregard menu events
        break;

      default:
        // ErrorExit("Unknown event type");
        break;
      }
    }
  }
};

Term::Term() : m_impl(new TermImpl) {}

Term::~Term() { delete m_impl; }

bool Term::Initialize() { return true; }

void Term::RawMode() { m_impl->RawMode(); }

SIZE Term::Size() const { return {}; }
short Term::AllocPair(int fg, int bg) { return {}; }

} // namespace term_screen
