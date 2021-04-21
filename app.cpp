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
            node->parent(nullptr);
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

    bool handleUserInput()
    {
        auto n = m_focused.lock();
        if (!n)
        {
            return false;
        }

        wint_t k = 0;
        int r = wget_wch(n->term->s->win, &k);

        // return handlechar(r, w);
        const char cmdstr[] = {(char)global::get_commandKey(), 0};
        static bool cmd = false;
#define KERR(i) (r == ERR && (i) == k)
#define KEY(i) (r == OK && (i) == k)
#define CODE(i) (r == KEY_CODE_YES && (i) == k)
#define INSCR (n->term->s->tos != n->term->s->off)
#define SB n->term->s->scrollbottom()
#define DO(s, t, a)                                                            \
    if (s == cmd && (t))                                                       \
    {                                                                          \
        a;                                                                     \
        cmd = false;                                                           \
        return true;                                                           \
    }

        DO(cmd, KERR(k), return false)
        DO(cmd, CODE(KEY_RESIZE), global::reshape(0, 0, LINES, COLS); SB)
        DO(false, KEY(global::get_commandKey()), return cmd = true)
        DO(false, KEY(0), n->term->safewrite("\000", 1); SB)
        DO(false, KEY(L'\n'), n->term->safewrite("\n"); SB)
        DO(false, KEY(L'\r'), n->term->safewrite(n->term->lnm ? "\r\n" : "\r");
           SB)
        DO(false, SCROLLUP && INSCR, n->term->s->scrollback(n->m_rect.h))
        DO(false, SCROLLDOWN && INSCR, n->term->s->scrollforward(n->m_rect.h))
        DO(false, RECENTER && INSCR, n->term->s->scrollbottom())
        DO(false, CODE(KEY_ENTER),
           n->term->safewrite(n->term->lnm ? "\r\n" : "\r");
           SB)
        DO(false, CODE(KEY_UP), sendarrow(n, "A"); SB);
        DO(false, CODE(KEY_DOWN), sendarrow(n, "B"); SB);
        DO(false, CODE(KEY_RIGHT), sendarrow(n, "C"); SB);
        DO(false, CODE(KEY_LEFT), sendarrow(n, "D"); SB);
        DO(false, CODE(KEY_HOME), n->term->safewrite("\033[1~"); SB)
        DO(false, CODE(KEY_END), n->term->safewrite("\033[4~"); SB)
        DO(false, CODE(KEY_PPAGE), n->term->safewrite("\033[5~"); SB)
        DO(false, CODE(KEY_NPAGE), n->term->safewrite("\033[6~"); SB)
        DO(false, CODE(KEY_BACKSPACE), n->term->safewrite("\177"); SB)
        DO(false, CODE(KEY_DC), n->term->safewrite("\033[3~"); SB)
        DO(false, CODE(KEY_IC), n->term->safewrite("\033[2~"); SB)
        DO(false, CODE(KEY_BTAB), n->term->safewrite("\033[Z"); SB)
        DO(false, CODE(KEY_F(1)), n->term->safewrite("\033OP"); SB)
        DO(false, CODE(KEY_F(2)), n->term->safewrite("\033OQ"); SB)
        DO(false, CODE(KEY_F(3)), n->term->safewrite("\033OR"); SB)
        DO(false, CODE(KEY_F(4)), n->term->safewrite("\033OS"); SB)
        DO(false, CODE(KEY_F(5)), n->term->safewrite("\033[15~"); SB)
        DO(false, CODE(KEY_F(6)), n->term->safewrite("\033[17~"); SB)
        DO(false, CODE(KEY_F(7)), n->term->safewrite("\033[18~"); SB)
        DO(false, CODE(KEY_F(8)), n->term->safewrite("\033[19~"); SB)
        DO(false, CODE(KEY_F(9)), n->term->safewrite("\033[20~"); SB)
        DO(false, CODE(KEY_F(10)), n->term->safewrite("\033[21~"); SB)
        DO(false, CODE(KEY_F(11)), n->term->safewrite("\033[23~"); SB)
        DO(false, CODE(KEY_F(12)), n->term->safewrite("\033[24~"); SB)
        DO(true, MOVE_UP, global::focus(n->m_rect.above()))
        DO(true, MOVE_DOWN, global::focus(n->m_rect.below()))
        DO(true, MOVE_LEFT, global::focus(n->m_rect.left()))
        DO(true, MOVE_RIGHT, global::focus(n->m_rect.right()))
        DO(true, MOVE_OTHER, global::focus_last())
        DO(true, HSPLIT, split(n, true))
        DO(true, VSPLIT, split(n, false))
        DO(true, DELETE_NODE, n->closed = true)
        DO(true, REDRAW, touchwin(stdscr); global::draw(); redrawwin(stdscr))
        DO(true, SCROLLUP, n->term->s->scrollback(n->m_rect.h))
        DO(true, SCROLLDOWN, n->term->s->scrollforward(n->m_rect.h))
        DO(true, RECENTER, n->term->s->scrollbottom())
        DO(true, KEY(global::get_commandKey()), n->term->safewrite(cmdstr, 1));
        char c[MB_LEN_MAX + 1] = {0};
        if (wctomb(c, k) > 0)
        {
            n->term->s->scrollbottom();
            n->term->safewrite(c);
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
                while (handleUserInput())
                    ;
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
