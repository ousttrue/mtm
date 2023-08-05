#pragma once
#include "curses_screen.h"
#include <memory>
#include <string>

class PosixProcess {
  struct PosixProcessImpl *m_impl;

  PosixProcess();

public:
  PosixProcess(const PosixProcess &) = delete;
  PosixProcess &operator=(const PosixProcess &) = delete;
  ~PosixProcess();
  static std::shared_ptr<PosixProcess> Fork(const SIZE &size,
                                            const char *term = nullptr,
                                            const char *shell = nullptr);
  static const char *GetTerm();
  static const char *GetShell();

  int FD() const;
  void Write(const char *b, size_t n);
  void Resize(const SIZE &size);
};
