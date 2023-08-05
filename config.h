#pragma once
#include <stddef.h>

#define COMMAND_KEY 'g'
#define SCROLLBACK 1000
#define DEFAULT_TERMINAL "screen-bce"
#define DEFAULT_256_COLOR_TERMINAL "screen-256color-bce"

/* The scrollback keys. */
#define SCROLLUP CODE(KEY_PPAGE)
#define SCROLLDOWN CODE(KEY_NPAGE)
#define RECENTER CODE(KEY_END)
/* The change focus keys. */
#define MOVE_UP CODE(KEY_UP)
#define MOVE_DOWN CODE(KEY_DOWN)
#define MOVE_RIGHT CODE(KEY_RIGHT)
#define MOVE_LEFT CODE(KEY_LEFT)
#define MOVE_OTHER KEY(L'o')

/* The split terminal keys. */
#define HSPLIT KEY(L'h')
#define VSPLIT KEY(L'v')

/* The delete terminal key. */
#define DELETE_NODE KEY(L'w')

/* The force redraw key. */
#define REDRAW KEY(L'l')

#define MAXMAP 0x7f
extern wchar_t CSET_US[MAXMAP]; /* "USASCII"...really just the null table */
extern wchar_t CSET_UK[MAXMAP];
extern wchar_t CSET_GRAPH[MAXMAP];
