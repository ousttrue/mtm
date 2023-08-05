#pragma once
#include <stddef.h>

#define COMMAND_KEY 'g'
#define SCROLLBACK 1000
#define DEFAULT_TERMINAL "screen-bce"
#define DEFAULT_256_COLOR_TERMINAL "screen-256color-bce"

#define MAXMAP 0x7f
extern wchar_t CSET_US[MAXMAP]; /* "USASCII"...really just the null table */
extern wchar_t CSET_UK[MAXMAP];
extern wchar_t CSET_GRAPH[MAXMAP];
