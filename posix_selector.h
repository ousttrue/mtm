#pragma once
#include <optional>
#include <span>
#include <stdint.h>

class Selector {
  Selector();

public:
  Selector(const Selector &) = delete;
  Selector &operator=(const Selector &) = delete;
  ~Selector() {}

  static Selector &Instance() {
    static Selector s_instance;
    return s_instance;
  }

  void Register(int fd);
  void Unregister(int fd);
  void Select();
  std::optional<std::span<char>> Read(int fd);
};
