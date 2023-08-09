#pragma once
#include "screen.h"
#include <vector>

struct PAIR {
  short fg = -1;
  short bg = -1;
};

class Term {

  std::vector<PAIR> m_pairs;

  Term();

public:
  Term(const Term &) = delete;
  Term &operator=(const Term &) = delete;
  ~Term();

  static Term &Insance() {
    static Term s_instance;
    return s_instance;
  }

  bool Initialize();
  void RawMode();
  SIZE Size() const;

  short AllocPair(int fg, int bg);
};
