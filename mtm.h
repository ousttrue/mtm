#pragma once

int mtm(const char *term, int commandKey);
void quit(int rc, const char *m);

inline int CTL(int x)
{
    return ((x)&0x1f);
}
