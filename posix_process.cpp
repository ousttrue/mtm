#include "posix_process.h"
#include "posix_selector.h"
#include <curses.h>
#include <pty.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>

#define DEFAULT_TERMINAL "screen-bce"
#define DEFAULT_256_COLOR_TERMINAL "screen-256color-bce"

struct PosixProcessImpl {
  int m_pty = -1;

  ~PosixProcessImpl() {
    Selector::Instance().Unregister(m_pty);
    close(m_pty);
  }

  void Write(const char *b, size_t n) {
    size_t w = 0;
    while (w < n) {
      ssize_t s = ::write(m_pty, b + w, n - w);
      if (s < 0 && errno != EINTR)
        return;
      else if (s < 0)
        s = 0;
      w += (size_t)s;
    }
  }
};

PosixProcess::PosixProcess() : m_impl(new PosixProcessImpl) {}

PosixProcess::~PosixProcess() {}

std::shared_ptr<PosixProcess>
PosixProcess::Fork(const SIZE &size, const char *term, const char *shell) {
  if (!term) {
    term = GetTerm();
  }
  if (!shell) {
    shell = GetShell();
  }

  auto ptr = std::shared_ptr<PosixProcess>(new PosixProcess);

  struct winsize ws = {
      .ws_row = size.Rows,
      .ws_col = size.Cols,
  };
  pid_t pid = forkpty(&ptr->m_impl->m_pty, NULL, NULL, &ws);
  if (pid < 0) {
    perror("forkpty");
    return {};
  } else if (pid == 0) {
    char buf[100] = {0};
    snprintf(buf, sizeof(buf) - 1, "%lu", (unsigned long)getppid());
    setsid();
    setenv("MTM", buf, 1);
    setenv("TERM", term, 1);
    signal(SIGCHLD, SIG_DFL);
    execl(shell, shell, NULL);
    return {};
  }

  Selector::Instance().Register(ptr->m_impl->m_pty);
  return ptr;
}

int PosixProcess::FD() const { return m_impl->m_pty; }

void PosixProcess::Write(const char *b, size_t n) { m_impl->Write(b, n); }

void PosixProcess::Resize(const SIZE &size) {
  struct winsize ws = {
      .ws_row = size.Rows,
      .ws_col = size.Cols,
  };
  ioctl(m_impl->m_pty, TIOCSWINSZ, &ws);
}

const char *PosixProcess::GetTerm() {
  const char *envterm = getenv("TERM");
  if (envterm) {
    return envterm;
  }
  if (COLORS >= 256 && !strstr(DEFAULT_TERMINAL, "-256color"))
    return DEFAULT_256_COLOR_TERMINAL;
  return DEFAULT_TERMINAL;
}

const char *PosixProcess::GetShell() /* Get the user's preferred shell. */
{
  if (getenv("SHELL"))
    return getenv("SHELL");
  struct passwd *pwd = getpwuid(getuid());
  if (pwd)
    return pwd->pw_shell;
  return "/bin/sh";
}
