#pragma once
#include <span>
#include <stdint.h>
#include <vector>

class InputStream {

  struct InputStreamImpl *m_impl = nullptr;

  InputStream();

public:
  InputStream(const InputStream &) = delete;
  InputStream &operator=(const InputStream &) = delete;
  ~InputStream();

  static InputStream &Instance() {
    static InputStream s_instance;
    return s_instance;
  }

  void Register(void *handle);
  void Unregister(void *handle);
  void Poll();
  void Enqueue(void *handle, std::span<const char> data);
  size_t Read(void *handle, std::vector<char> &buf);
};
