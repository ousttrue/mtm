#include "curses_term.h"
#pragma once

struct VtContext
{
    void *p;
    wchar_t w;
    wchar_t iw;
    int argc;
    int *argv;
    const wchar_t *osc;

    std::tuple<int, int, int, int, int, int, int, int, int> get() const;
};
