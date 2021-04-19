#pragma once

inline int CTL(int x)
{
    return ((x)&0x1f);
}

namespace global
{

void set_term(const char *term);
const char *get_term(void);

void set_commandkey(int k);
int get_commandKey();

} // namespace global
