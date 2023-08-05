#pragma once

class Term {

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
};
