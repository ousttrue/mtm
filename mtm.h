#pragma once
#include <memory>


class mtm
{
    mtm();

public:
    static std::unique_ptr<mtm> create();
    ~mtm();
    int run();
};
