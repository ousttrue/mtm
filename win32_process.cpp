#include "child_process.h"
#include <Windows.h>
#include <strsafe.h>
#include <vector>

namespace term_screen {

struct ProcessImpl {
  HANDLE m_outputReadSide = nullptr;
  HANDLE m_inputWriteSide = nullptr;
  HANDLE m_hPC = nullptr;
  STARTUPINFOEXA m_si = {};
  PROCESS_INFORMATION m_pi = {};

  ProcessImpl(HANDLE outputReadSide, HANDLE inputWriteSide)
      : m_outputReadSide(outputReadSide), m_inputWriteSide(inputWriteSide) {}
  ~ProcessImpl() {
    ClosePseudoConsole(m_hPC);
    m_hPC = nullptr;
    CloseHandle(m_outputReadSide);
    m_outputReadSide = nullptr;
    CloseHandle(m_inputWriteSide);
    m_inputWriteSide = nullptr;

    ZeroMemory(&m_si, sizeof(m_si));
    ZeroMemory(&m_pi, sizeof(m_pi));
  }

  bool CreateConPty(const COORD &size, HANDLE inputReadSide,
                    HANDLE outputWriteSide) {
    auto hr =
        CreatePseudoConsole(size, inputReadSide, outputWriteSide, 0, &m_hPC);
    // CloseHandle(inputReadSide);
    // CloseHandle(outputWriteSide);
    if (FAILED(hr)) {
      // return hr;
      return {};
    }
    return true;
  }

  bool CreateProcess(const char *shell) {
    // Prepare Startup Information structure
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

    // Call CreateProcess
    if (!::CreateProcessA(NULL, cmdLineMutable, NULL, NULL, FALSE,
                          EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
                          &m_si.StartupInfo, &m_pi)) {
      HeapFree(GetProcessHeap(), 0, cmdLineMutable);
      // return HRESULT_FROM_WIN32(GetLastError());
      return {};
    }

    // Close handles to the child process and its primary thread.
    // Some applications might keep these handles to monitor the status
    // of the child process, for example.

    CloseHandle(m_pi.hProcess);
    // CloseHandle(m_pi.hThread);

    // Close handles to the stdin and stdout pipes no longer needed by the child
    // process. If they are not explicitly closed, there is no way to recognize
    // that the child process has ended.

    return true;
  }

  size_t ReadSync(char *buf, DWORD bufSize) {
    auto wait = WaitForSingleObject(m_outputReadSide, 0);
    if (wait == WAIT_TIMEOUT) {
      return 0;
    }

    if (wait == WAIT_OBJECT_0) {
      DWORD size;
      if (!ReadFile(m_outputReadSide, buf, bufSize, &size, nullptr)) {
        auto dw = GetLastError();
        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&lpMsgBuf, 0, NULL);
        auto lpDisplayBuf = (LPVOID)LocalAlloc(
            LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + 40) * sizeof(TCHAR));
        StringCchPrintf((LPTSTR)lpDisplayBuf,
                        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                        TEXT("failed with error %d: %s"), dw, lpMsgBuf);
        MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);
        return 0;
      }

      DWORD code;
      if (GetExitCodeProcess(m_pi.hProcess, &code)) {
        CloseHandle(m_outputReadSide);
        m_outputReadSide = nullptr;
      }

      return size;
    }

    CloseHandle(m_outputReadSide);
    m_outputReadSide = nullptr;
    return 0;
  }

  void BeginDrain(const std::function<void(const char *, size_t)> &callback) {
    while (auto handle = m_outputReadSide) {
      char buf[8192];
      DWORD size;
      if (ReadFile(handle, buf, sizeof(buf), &size, nullptr) && size > 0) {
        // InputStream::Instance().Enqueue(handle, {buf, size});
        // std::scoped_lock lock(m_mutex);
        // auto current = m_drainBuffer.size();
        // m_drainBuffer.resize(current + size);
        callback(buf, size);
      }
    }
  }

  void Resize(const COORD &size) { ResizePseudoConsole(m_hPC, size); }
};

//
// Process
//
Process::Process() {}

Process::~Process() { delete m_impl; }
std::shared_ptr<Process> Process::Fork(const SIZE &size, const char *shell,
                                       const char *term) {

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
  ptr->m_impl = new ProcessImpl(OutputReadSide, InputWriteSide);
  if (!ptr->m_impl->CreateConPty(
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

  CloseHandle(inputReadSide);
  CloseHandle(outputWriteSide);

  return ptr;
}

const char *Process::GetTerm() { return {}; }
const char *Process::GetShell() { return "cmd.exe"; }

void *Process::Handle() const { return m_impl->m_outputReadSide; }

void Process::BeginDrain(
    const std::function<void(const char *, size_t)> &callback) {
  m_impl->BeginDrain(callback);
}

size_t Process::ReadSync(char *buf, size_t buf_size) {
  return m_impl->ReadSync(buf, buf_size);
}

void Process::Write(const char *b, size_t n) {
  DWORD size;
  WriteFile(m_impl->m_inputWriteSide, b, n, &size, nullptr);
}

void Process::WriteString(const char *b) { Write(b, strlen(b)); }

void Process::Resize(const SIZE &size) {
  m_impl->Resize({.X = (short)size.Cols, .Y = (short)size.Rows});
}

} // namespace term_screen
