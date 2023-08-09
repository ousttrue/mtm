#include "process.h"

struct ProcessImpl {};

Process::Process() : m_impl(new ProcessImpl) {}

Process::~Process() { delete m_impl; }
std::shared_ptr<Process> Process::Fork(const SIZE &size, const char *term,
                                       const char *shell) {
  return {};
}
const char *Process::GetTerm() { return {}; }
const char *Process::GetShell() { return {}; }

void *Process::Handle() const { return {}; }
void Process::Write(const char *b, size_t n) {}
void Process::WriteString(const char *b) {}
void Process::Resize(const SIZE &size) {}

