#include "app.h"
#include "node.h"
#include "scrn.h"
#include "selector.h"
#include "curses_term.h"
#include <climits>
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

static void sendarrow(const std::shared_ptr<NODE> &n, const char *k)
{
    char buf[100] = {0};
    snprintf(buf, sizeof(buf) - 1, "\033%s%s", n->term->pnm ? "O" : "[", k);
    n->term->safewrite(buf);
}

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

        auto rect = Rect(0, 0, LINES, COLS);
        m_root = std::make_shared<NODE>(rect);
        m_root->term = new_term(rect);
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
        if (node)
        {
            node->reshape(Rect(0, 0, LINES, COLS));
        }
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

        auto view = n->findViewNode();
        if (view)
        {
            m_lastfocused = m_focused;
            m_focused = view;
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

    enum Command
    {
        ON,
        OFF,
        ANY,
    };

    bool cmd = false;

    bool handleUserInput(const std::shared_ptr<NODE> &n,
                         const std::unique_ptr<CursesTerm> &term)
    {
        wint_t k = 0;
        int r = wget_wch(term->s->win, &k);
        if (r == ERR)
        {
            return false;
        }

#define KEY(i) (r == OK && (i) == k)
#define CODE(i) (r == KEY_CODE_YES && (i) == k)
#define INSCR (n->term->s->tos != n->term->s->off)

#define DO(s, t, a)                                                            \
    if (s == cmd && (t))                                                       \
    {                                                                          \
        a;                                                                     \
        cmd = false;                                                           \
        return true;                                                           \
    }

        DO(cmd, CODE(KEY_RESIZE), global::reshape(0, 0, LINES, COLS); term->s->scrollbottom())
        DO(false, KEY(global::get_commandKey()), return cmd = true)
        DO(false, KEY(0), term->safewrite("\000", 1); term->s->scrollbottom())
        DO(false, KEY(L'\n'), term->safewrite("\n"); term->s->scrollbottom())
        DO(false, KEY(L'\r'), term->safewrite(term->lnm ? "\r\n" : "\r"); term->s->scrollbottom())
        DO(false, SCROLLUP && INSCR, term->scrollback())
        DO(false, SCROLLDOWN && INSCR, term->scrollforward())
        DO(false, RECENTER && INSCR, term->s->scrollbottom())
        DO(false, CODE(KEY_ENTER), term->safewrite(term->lnm ? "\r\n" : "\r");
           term->s->scrollbottom())
        DO(false, CODE(KEY_UP), sendarrow(n, "A"); term->s->scrollbottom());
        DO(false, CODE(KEY_DOWN), sendarrow(n, "B"); term->s->scrollbottom());
        DO(false, CODE(KEY_RIGHT), sendarrow(n, "C"); term->s->scrollbottom());
        DO(false, CODE(KEY_LEFT), sendarrow(n, "D"); term->s->scrollbottom());
        DO(false, CODE(KEY_HOME), term->safewrite("\033[1~"); term->s->scrollbottom())
        DO(false, CODE(KEY_END), term->safewrite("\033[4~"); term->s->scrollbottom())
        DO(false, CODE(KEY_PPAGE), term->safewrite("\033[5~"); term->s->scrollbottom())
        DO(false, CODE(KEY_NPAGE), term->safewrite("\033[6~"); term->s->scrollbottom())
        DO(false, CODE(KEY_BACKSPACE), term->safewrite("\177"); term->s->scrollbottom())
        DO(false, CODE(KEY_DC), term->safewrite("\033[3~"); term->s->scrollbottom())
        DO(false, CODE(KEY_IC), term->safewrite("\033[2~"); term->s->scrollbottom())
        DO(false, CODE(KEY_BTAB), term->safewrite("\033[Z"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(1)), term->safewrite("\033OP"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(2)), term->safewrite("\033OQ"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(3)), term->safewrite("\033OR"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(4)), term->safewrite("\033OS"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(5)), term->safewrite("\033[15~"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(6)), term->safewrite("\033[17~"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(7)), term->safewrite("\033[18~"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(8)), term->safewrite("\033[19~"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(9)), term->safewrite("\033[20~"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(10)), term->safewrite("\033[21~"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(11)), term->safewrite("\033[23~"); term->s->scrollbottom())
        DO(false, CODE(KEY_F(12)), term->safewrite("\033[24~"); term->s->scrollbottom())
        DO(true, MOVE_UP, global::focus(term->m_rect.above()))
        DO(true, MOVE_DOWN, global::focus(term->m_rect.below()))
        DO(true, MOVE_LEFT, global::focus(term->m_rect.left()))
        DO(true, MOVE_RIGHT, global::focus(term->m_rect.right()))
        DO(true, MOVE_OTHER, global::focus_last())
        DO(true, HSPLIT, n->split(true))
        DO(true, VSPLIT, n->split(false))
        DO(true, DELETE_NODE, n->closed = true)
        DO(true, REDRAW, touchwin(stdscr); global::draw(); redrawwin(stdscr))
        DO(true, SCROLLUP, term->scrollback())
        DO(true, SCROLLDOWN, term->scrollforward())
        DO(true, RECENTER, term->s->scrollbottom())
        const char cmdstr[] = {(char)global::get_commandKey(), 0};
        DO(true, KEY(global::get_commandKey()), term->safewrite(cmdstr, 1));
        char c[MB_LEN_MAX + 1] = {0};
        if (wctomb(c, k) > 0)
        {
            term->s->scrollbottom();
            term->safewrite(c);
        }
        return cmd = false, true;
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
                while (true)
                {
                    auto focus = m_focused.lock();
                    if (focus)
                    {
                        if (!handleUserInput(focus, focus->term))
                        {
                            break;
                        }
                    }
                }
            }

            //
            // read pty and process vt
            //
            selector::select();
            m_root->process();
            if (m_root->closed)
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
