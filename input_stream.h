#pragma once
#include <optional>
#include <span>
#include <stdint.h>

class InputStream {
  InputStream();

public:
  InputStream(const InputStream &) = delete;
  InputStream &operator=(const InputStream &) = delete;
  ~InputStream() {}

  static InputStream &Instance() {
    static InputStream s_instance;
    return s_instance;
  }

  void Register(void *handle);
  void Unregister(void *handle);
  void Poll();
  std::optional<std::span<char>> Read(void *handle);
};
