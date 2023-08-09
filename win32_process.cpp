#include "child_process.h"
#include "input_stream.h"
#include <Windows.h>
#include <thread>
#include <vector>

namespace term_screen {

struct ProcessImpl {
  ::HANDLE OutputReadSide = nullptr;
  ::HANDLE InputWriteSide = nullptr;
  ::HANDLE m_hPC = nullptr;
  ::STARTUPINFOEXA m_si = {};
  std::thread m_readThread;

  ProcessImpl(HANDLE outputReadSide, HANDLE inputWriteSide)
      : OutputReadSide(outputReadSide), InputWriteSide(inputWriteSide) {}
  ~ProcessImpl() {
    ClosePseudoConsole(m_hPC);
    m_hPC = nullptr;
    CloseHandle(OutputReadSide);
    OutputReadSide = nullptr;
    CloseHandle(InputWriteSide);
    InputWriteSide = nullptr;
    m_readThread.join();
  }

  bool ConPty(const COORD &size, HANDLE inputReadSide, HANDLE outputWriteSide) {
    auto hr =
        CreatePseudoConsole(size, inputReadSide, outputWriteSide, 0, &m_hPC);
    if (FAILED(hr)) {
      // return hr;
      return {};
    }
    return true;
  }

  bool CreateProcess(const char *shell) {
    // Prepare Startup Information structure
    ZeroMemory(&m_si, sizeof(m_si));
    m_si.StartupInfo.cb = sizeof(STARTUPINFOEX);

    // Discover the size required for the list
    size_t bytesRequired;
    InitializeProcThreadAttributeList(NULL, 1, 0, &bytesRequired);

    // Allocate memory to represent the list
    m_si.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
        GetProcessHeap(), 0, bytesRequired);
    if (!m_si.lpAttributeList) {
      // return E_OUTOFMEMORY;
      return {};
    }

    // Initialize the list memory location
    if (!InitializeProcThreadAttributeList(m_si.lpAttributeList, 1, 0,
                                           &bytesRequired)) {
      HeapFree(GetProcessHeap(), 0, m_si.lpAttributeList);
      // return HRESULT_FROM_WIN32(GetLastError());
      return {};
    }

    // Set the pseudoconsole information into the list
    if (!UpdateProcThreadAttribute(m_si.lpAttributeList, 0,
                                   PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, m_hPC,
                                   sizeof(m_hPC), NULL, NULL)) {
      HeapFree(GetProcessHeap(), 0, m_si.lpAttributeList);
      // return HRESULT_FROM_WIN32(GetLastError());
      return {};
    }

    // Create mutable text string for CreateProcessW command line string.
    const size_t charsRequired = strlen(shell) + 1; // +1 null terminator
    auto cmdLineMutable =
        (char *)HeapAlloc(GetProcessHeap(), 0, sizeof(char) * charsRequired);
    if (!cmdLineMutable) {
      // return E_OUTOFMEMORY;
      return {};
    }
    std::copy(shell, shell + charsRequired, cmdLineMutable);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    // Call CreateProcess
    if (!::CreateProcessA(NULL, cmdLineMutable, NULL, NULL, FALSE,
                          EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
                          &m_si.StartupInfo, &pi)) {
      HeapFree(GetProcessHeap(), 0, cmdLineMutable);
      // return HRESULT_FROM_WIN32(GetLastError());
      return {};
    }

    m_readThread = std::thread([self = this]() { self->Drain(); });

    return true;
  }

  void Drain() {
    while (auto handle = OutputReadSide) {
      char buf[8192];
      DWORD size;
      if (ReadFile(handle, buf, sizeof(buf), &size, nullptr) && size > 0) {
        InputStream::Instance().Enqueue(handle, {buf, size});
      }
    }
  }

  void Resize(const COORD &size) { ResizePseudoConsole(m_hPC, size); }
};

Process::Process() {}

Process::~Process() { delete m_impl; }
std::shared_ptr<Process> Process::Fork(const SIZE &size, const char *term,
                                       const char *shell) {

  // - Hold onto these and use them for communication with the child through the
  // pseudoconsole.
  HANDLE OutputReadSide = nullptr;
  HANDLE InputWriteSide = nullptr;

  // - Close these after CreateProcess of child application with pseudoconsole
  // object.
  HANDLE inputReadSide = nullptr;
  HANDLE outputWriteSide = nullptr;
  // +----+ W        R +-----+
  // |host| ===Input=> |child|
  // |    | <=Output== |     |
  // +----+ R        W +-----+

  if (!CreatePipe(&inputReadSide, &InputWriteSide, NULL, 0)) {
    // return HRESULT_FROM_WIN32(GetLastError());
    return {};
  }
  if (!CreatePipe(&OutputReadSide, &outputWriteSide, NULL, 0)) {
    // return HRESULT_FROM_WIN32(GetLastError());
    return {};
  }

  // Create communication channels
  std::shared_ptr<Process> ptr(new Process);
  ptr->m_impl = new ProcessImpl(InputWriteSide, OutputReadSide);
  if (!ptr->m_impl->ConPty(
          COORD{
              .X = (short)size.Cols,
              .Y = (short)size.Rows,
          },
          inputReadSide, outputWriteSide)) {
    return {};
  }

  if (!ptr->m_impl->CreateProcess(shell)) {
    return {};
  }

  return ptr;
}

const char *Process::GetTerm() { return {}; }
const char *Process::GetShell() { return "cmd.exe"; }

void *Process::Handle() const { return m_impl->OutputReadSide; }

void Process::Write(const char *b, size_t n) {
  DWORD size;
  WriteFile(m_impl->InputWriteSide, b, n, &size, nullptr);
}

void Process::WriteString(const char *b) { Write(b, strlen(b)); }

void Process::Resize(const SIZE &size) {
  m_impl->Resize({.X = (short)size.Cols, .Y = (short)size.Rows});
}

} // namespace term_screen
