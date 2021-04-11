#pragma once
#include <memory>

inline int CTL(int x)
{
    return ((x)&0x1f);
}

void set_commandkey(int k);

int fork_setup(struct VTPARSER *vp, void *p, int *pt, int h, int w);
void sendarrow(const std::shared_ptr<struct NODE> &n, const char *k);
bool handlechar(int r, int k); /* Handle a single input character. */

void set_term(const char *term);
const char *get_term(void);
