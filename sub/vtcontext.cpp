#include "vtcontext.h"
#include "curses_term.h"
#include "node.h"
#include "curses_window.h"
#include <curses.h>

// (END)HANDLER   - Declare/end a handler function
 void VtContext::end()
{
    auto term = (CursesTerm *)this->p;
    term->repc = 0;
}
