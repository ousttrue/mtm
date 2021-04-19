#include "app.h"
#include "node.h"
#include "selector.h"
#include "curses_term.h"
#include <cstring>
#include <exception>
#include <curses.h>
#include <string>
#include "pair.h"

inline int CTL(int x)
{
    return ((x)&0x1f);
}

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
    g_commandkey = CTL(k);
}

int get_commandKey()
{
    return g_commandkey;
}

//
// Node manipulation
//

class App
{
    std::shared_ptr<NODE> m_root;
    std::weak_ptr<NODE> m_focused;
    std::weak_ptr<NODE> m_lastfocused;

public:
    App()
    {
        raw();
        noecho();
        nonl();
        intrflush(stdscr, FALSE);
        start_color();
        use_default_colors();
        start_pairs();

        m_root = newview(Rect(0, 0, LINES, COLS));
        focus(m_root);
    }

    ~App()
    {
        endwin();
    }

    void quit()
    {
        m_root = nullptr;
    }
    void root(const std::shared_ptr<NODE> &node)
    {
        m_root = node;
    }
    std::shared_ptr<NODE> root() const
    {
        return m_root;
    }

    void focus(const std::shared_ptr<NODE> &n) /* Focus a node. */
    {
        if (!n)
        {
            return;
        }

        if (n->isView())
        {
            m_lastfocused = m_focused;
            m_focused = n;
        }
        else
        {
            focus(n->c1 ? n->c1 : n->c2);
        }
    }
    std::shared_ptr<NODE> focus() const
    {
        return m_focused.lock();
    }
    void focus_last()
    {
        if (auto last = m_lastfocused.lock())
        {
            focus(last);
        }
    }

    int run()
    {

        //
        // main loop
        //
        for (int i = 0; true; ++i)
        {
            //
            // process all user input
            //
            {
                auto f = m_focused.lock();
                while (f->term->handleUserInput())
                    ;
            }

            //
            // read pty and process vt
            //
            selector::select();
            m_root->processVT();
            if (!m_root)
            {
                break;
            }

            //
            // update visual
            //
            {
                // cursor for focused
                auto f = m_focused.lock();
                f->term->fixCursor();
                // f->draw();

                m_root->draw();
                doupdate();
            }
        }

        return EXIT_SUCCESS;
    }
};
std::unique_ptr<App> g_app;

int run()
{
    if (!initscr())
    {
        // quit(EXIT_FAILURE, "could not initialize terminal");
        return 1;
    }

    g_app = std::make_unique<App>();
    return g_app->run();
}

// focus
std::shared_ptr<NODE> focus()
{
    return g_app->focus();
}
void focus(const std::shared_ptr<NODE> &node)
{
    g_app->focus(node);
}
void focus(YX yx)
{
    auto found = root()->findnode(yx);
    g_app->focus(found);
}
void focus_last()
{
    g_app->focus_last();
}

// root
void quit()
{
    g_app->quit();
}

std::shared_ptr<NODE> root()
{
    return g_app->root();
}
void root(const std::shared_ptr<NODE> &node)
{
    g_app->root(node);
}
void reshape(int y, int x, int h, int w)
{
    g_app->root()->reshape({y, x, h, w});
}
void draw()
{
    g_app->root()->draw();
}

} // namespace global
