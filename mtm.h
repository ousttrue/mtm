#pragma once
#include <memory>


class mtm
{
    mtm();

public:
    static std::unique_ptr<mtm> create(const char *term, int commandKey);
    ~mtm();
    int run();
};
