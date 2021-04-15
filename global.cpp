#include "global.h"
#include <cstring>
#include <curses.h>
#include <string>

namespace global
{
std::string g_term;

void set_term(const char *term)
{
    if (term)
    {
        g_term = term;
    }
}

#define DEFAULT_TERMINAL "screen-bce"
#define DEFAULT_256_COLOR_TERMINAL "screen-256color-bce"

const char *get_term(void)
{
    const char *envterm = getenv("TERM");
    if (!g_term.empty())
        return g_term.c_str();
    if (envterm && COLORS >= 256 && !strstr(DEFAULT_TERMINAL, "-256color"))
        return DEFAULT_256_COLOR_TERMINAL;
    return DEFAULT_TERMINAL;
}

/* The default command prefix key, when modified by cntrl.
 * This can be changed at runtime using the '-c' flag.
 */
#define COMMAND_KEY 'g'

static int g_commandkey = CTL(COMMAND_KEY);

void set_commandkey(int k)
{
    g_commandkey = k;
}

int get_commandKey()
{
    return g_commandkey;
}

} // namespace global
