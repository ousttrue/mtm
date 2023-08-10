#pragma once
#include "screen.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace term_screen {

class Process {
  struct ProcessImpl *m_impl;

  Process();

public:
  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;
  ~Process();
  static std::shared_ptr<Process> Fork(const SIZE &size,
                                       const char *shell = nullptr,
                                       const char *term = nullptr);
  static const char *GetTerm();
  static const char *GetShell();
  void *Handle() const;

  void BeginDrain(const std::function<void(const char *, size_t)> &callback);
  size_t ReadSync(char *buf, size_t buf_size);
  void Write(const char *b, size_t n);
  void WriteString(const char *b);

  void Resize(const SIZE &size);
};

} // namespace term_screen
