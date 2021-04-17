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

    // PD(n, d)       - Parameter n, with default d.
    int PD(int x, int d) const
    {
        return (argc < (x) || !argv ? (d) : argv[(x)]);
    }

    // P0(n)          - Parameter n, default 0.
    int P0(int x) const
    {
        return PD(x, 0);
    }

    // P1(n)          - Parameter n, default 1.
    int P1(int x) const
    {
        return (!P0(x) ? 1 : P0(x));
    }
};
