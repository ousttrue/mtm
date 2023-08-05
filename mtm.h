#pragma once
#include <memory>
#include <sys/select.h>
#include <vector>

extern fd_set fds;
extern int commandkey;
#define CTL(x) ((x)&0x1f)
extern const char *term;

// Open a new view
struct NODE;
struct POS;
struct SIZE;
NODE *newview(const POS &pos, const SIZE &size);

// stdin
extern int nfds;
