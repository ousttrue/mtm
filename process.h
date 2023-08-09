#pragma once
#include "screen.h"
#include <memory>
#include <string>

class Process {
  struct ProcessImpl *m_impl;

  Process();

public:
  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;
  ~Process();
  static std::shared_ptr<Process> Fork(const SIZE &size,
                                       const char *term = nullptr,
                                       const char *shell = nullptr);
  static const char *GetTerm();
  static const char *GetShell();

  void *Handle() const;
  void Write(const char *b, size_t n);
  void WriteString(const char *b);

  void Resize(const SIZE &size);
};
