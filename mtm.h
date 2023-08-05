#pragma once
#include <sys/select.h>

extern fd_set fds;
extern int commandkey;
#define CTL(x) ((x)&0x1f)
extern const char *term;

void quit(int rc, const char *m); /* Shut down MTM. */

typedef struct NODE NODE;
NODE *newview(NODE *p, int y, int x, int h, int w); /* Open a new view. */
extern NODE *root;
void focus(NODE *n); /* Focus a node. */
void draw(NODE *n);
void run(void); /* Run MTM. */
