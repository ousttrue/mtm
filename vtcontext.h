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

    CursesTerm *term() const
    {
        return (CursesTerm *)p;
    }

    void end();

    std::tuple<int, int, int, int, int, int, int, int, int> get() const;

    int PD(int x, int d) const
    {
        return (argc < (x) || !argv ? (d) : argv[(x)]);
    }
    int P0(int x) const
    {
        return PD(x, 0);
    }
    int P1(int x) const
    {
        return (!P0(x) ? 1 : P0(x));
    }
};
