#include "posix_selector.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

static int g_nfds = 1; /* stdin */
static fd_set g_fds;
static char g_iobuf[BUFSIZ];

Selector::Selector() { FD_SET(STDIN_FILENO, &g_fds); }

void Selector::Register(int fd) {
  FD_SET(fd, &g_fds);
  fcntl(fd, F_SETFL, O_NONBLOCK);
  g_nfds = fd > g_nfds ? fd : g_nfds;
}

void Selector::Unregister(int fd) { FD_CLR(fd, &g_fds); }

void Selector::Select() {
  fd_set sfds = g_fds;
  if (select(g_nfds + 1, &sfds, nullptr, nullptr, nullptr) < 0) {
    FD_ZERO(&sfds);
  }
}

std::optional<std::span<char>> Selector::Read(int fd) {
  if (fd <= 0) {
    // error
    return std::nullopt;
  }
  if (!FD_ISSET(fd, &g_fds)) {
    // empty
    return std::span<char>{};
  }

  auto r = read(fd, g_iobuf, std::size(g_iobuf));
  if (r > 0) {
    return std::span<char>(g_iobuf, r);
  }

  if (errno != EINTR && errno != EWOULDBLOCK) {
    // error
    return std::nullopt;
  }

  // empty
  return std::span<char>{};
}
