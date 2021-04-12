#pragma once
#include <memory>

inline int CTL(int x)
{
    return ((x)&0x1f);
}

void set_commandkey(int k);

void sendarrow(const std::shared_ptr<struct NODE> &n, const char *k);
bool handlechar(int r, int k); /* Handle a single input character. */

void set_term(const char *term);
const char *get_term(void);
void vp_initialize(struct VTPARSER *vp, void *p);
