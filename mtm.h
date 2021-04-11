#pragma once
#include <memory>

inline int CTL(int x)
{
    return ((x)&0x1f);
}

class mtm
{
    mtm();

public:
    static std::unique_ptr<mtm> create(const char *term, int commandKey);
    ~mtm();
    int run();
};
