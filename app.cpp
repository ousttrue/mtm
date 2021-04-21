#include "app.h"
#include "node.h"
#include "scrn.h"
#include "selector.h"
#include "curses_term.h"
#include "pair.h"
#include <bits/types/wint_t.h>
#include <climits>
#include <cstring>
#include <exception>
#include <curses.h>
#include <string>
#include <unordered_map>

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
static void sendarrow(const std::shared_ptr<NODE> &n, const char *k)
{
    char buf[100] = {0};
    snprintf(buf, sizeof(buf) - 1, "\033%s%s", n->term->pnm ? "O" : "[", k);
    n->term->safewrite(buf);
}

using KeyCallbackFunc = std::function<void(
    const std::shared_ptr<NODE> &n, const std::unique_ptr<CursesTerm> &term)>;

class App
{
    std::shared_ptr<NODE> m_root;
    std::weak_ptr<NODE> m_focused;
    std::weak_ptr<NODE> m_lastfocused;

    std::unordered_map<wint_t, KeyCallbackFunc> m_keyCodeMap;
    std::unordered_map<wint_t, KeyCallbackFunc> m_okMap;
    std::unordered_map<wint_t, KeyCallbackFunc> m_cmdKeyCodeMap;
    std::unordered_map<wint_t, KeyCallbackFunc> m_cmdOkMap;

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

        m_cmdKeyCodeMap.insert(
            {KEY_UP, [](const std::shared_ptr<NODE> &n,
                        const std::unique_ptr<CursesTerm> &term) {
                 global::focus(term->m_rect.above());
             }});
        m_cmdKeyCodeMap.insert(
            {KEY_DOWN, [](const std::shared_ptr<NODE> &n,
                          const std::unique_ptr<CursesTerm> &term) {
                 global::focus(term->m_rect.below());
             }});
        m_cmdKeyCodeMap.insert(
            {KEY_LEFT, [](const std::shared_ptr<NODE> &n,
                          const std::unique_ptr<CursesTerm> &term) {
                 global::focus(term->m_rect.left());
             }});
        m_cmdKeyCodeMap.insert(
            {KEY_RIGHT, [](const std::shared_ptr<NODE> &n,
                           const std::unique_ptr<CursesTerm> &term) {
                 global::focus(term->m_rect.right());
             }});
        m_cmdKeyCodeMap.insert(
            {KEY_PPAGE, [](const std::shared_ptr<NODE> &n,
                           const std::unique_ptr<CursesTerm> &term) {
                 term->scrollback();
             }});
        m_cmdKeyCodeMap.insert(
            {KEY_NPAGE, [](const std::shared_ptr<NODE> &n,
                           const std::unique_ptr<CursesTerm> &term) {
                 term->scrollforward();
             }});
        m_cmdKeyCodeMap.insert(
            {KEY_END, [](const std::shared_ptr<NODE> &n,
                         const std::unique_ptr<CursesTerm> &term) {
                 term->s->scrollbottom();
             }});

        m_cmdOkMap.insert({L'o', [](const std::shared_ptr<NODE> &n,
                                    const std::unique_ptr<CursesTerm> &term) {
                               global::focus_last();
                           }});
        m_cmdOkMap.insert({L'h', [](const std::shared_ptr<NODE> &n,
                                    const std::unique_ptr<CursesTerm> &term) {
                               n->split(true);
                           }});
        m_cmdOkMap.insert({L'v', [](const std::shared_ptr<NODE> &n,
                                    const std::unique_ptr<CursesTerm> &term) {
                               n->split(false);
                           }});
        m_cmdOkMap.insert({L'w', [](const std::shared_ptr<NODE> &n,
                                    const std::unique_ptr<CursesTerm> &term) {
                               n->closed = true;
                           }});
        m_cmdOkMap.insert({L'l', [](const std::shared_ptr<NODE> &n,
                                    const std::unique_ptr<CursesTerm> &term) {
                               touchwin(stdscr);
                               global::draw();
                               redrawwin(stdscr);
                           }});

        m_okMap.insert({0, [](const std::shared_ptr<NODE> &n,
                              const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite("\000", 1);
                            term->s->scrollbottom();
                        }});
        m_okMap.insert({L'\n', [](const std::shared_ptr<NODE> &n,
                                  const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite("\n");
                            term->s->scrollbottom();
                        }});
        m_okMap.insert({L'\r', [](const std::shared_ptr<NODE> &n,
                                  const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite(term->lnm ? "\r\n" : "\r");
                            term->s->scrollbottom();
                        }});

        // DO(CODE(KEY_PPAGE) && term->s->INSCR(), term->scrollback())
        // DO(CODE(KEY_NPAGE) && term->s->INSCR(), term->scrollforward())
        // DO(CODE(KEY_END) && term->s->INSCR(), term->s->scrollbottom())
        m_keyCodeMap.insert({KEY_ENTER, [](const std::shared_ptr<NODE> &n,
                                      const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite(term->lnm ? "\r\n" : "\r");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_UP, [](const std::shared_ptr<NODE> &n,
                                   const std::unique_ptr<CursesTerm> &term) {
                            sendarrow(n, "A");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_DOWN, [](const std::shared_ptr<NODE> &n,
                                     const std::unique_ptr<CursesTerm> &term) {
                            sendarrow(n, "B");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_RIGHT, [](const std::shared_ptr<NODE> &n,
                                      const std::unique_ptr<CursesTerm> &term) {
                            sendarrow(n, "C");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_LEFT, [](const std::shared_ptr<NODE> &n,
                                     const std::unique_ptr<CursesTerm> &term) {
                            sendarrow(n, "D");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_HOME, [](const std::shared_ptr<NODE> &n,
                                     const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite("\033[1~");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_END, [](const std::shared_ptr<NODE> &n,
                                    const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite("\033[4~");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_PPAGE, [](const std::shared_ptr<NODE> &n,
                                      const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite("\033[5~");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_NPAGE, [](const std::shared_ptr<NODE> &n,
                                      const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite("\033[6~");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert(
            {KEY_BACKSPACE, [](const std::shared_ptr<NODE> &n,
                               const std::unique_ptr<CursesTerm> &term) {
                 term->safewrite("\177");
                 term->s->scrollbottom();
             }});
        m_keyCodeMap.insert({KEY_DC, [](const std::shared_ptr<NODE> &n,
                                   const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite("\033[3~");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_IC, [](const std::shared_ptr<NODE> &n,
                                   const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite("\033[2~");
                            term->s->scrollbottom();
                        }});
        m_keyCodeMap.insert({KEY_BTAB, [](const std::shared_ptr<NODE> &n,
                                     const std::unique_ptr<CursesTerm> &term) {
                            term->safewrite("\033[Z");
                            term->s->scrollbottom();
                        }});

        //     DO(CODE(KEY_F(1)),term->safewrite("\033OP");
        //     term->s->scrollbottom();
        // }});
        // DO(CODE(KEY_F(2)), term->safewrite("\033OQ");term->s->scrollbottom())
        // DO(CODE(KEY_F(3)),term->safewrite("\033OR"); term->s->scrollbottom())
        // DO(CODE(KEY_F(4)), term->safewrite("\033OS");term->s->scrollbottom())
        // DO(CODE(KEY_F(5)),term->safewrite("\033[15~");
        // term->s->scrollbottom()) DO(CODE(KEY_F(6)),
        // term->safewrite("\033[17~");term->s->scrollbottom())
        // DO(CODE(KEY_F(7)),term->safewrite("\033[18~");
        // term->s->scrollbottom()) DO(CODE(KEY_F(8)),
        // term->safewrite("\033[19~");term->s->scrollbottom())
        // DO(CODE(KEY_F(9)),term->safewrite("\033[20~");
        // term->s->scrollbottom()) DO(CODE(KEY_F(10)),
        // term->safewrite("\033[21~");term->s->scrollbottom())
        // DO(CODE(KEY_F(11)),
        // term->safewrite("\033[23~");term->s->scrollbottom())
        // DO(CODE(KEY_F(12)),
        // term->safewrite("\033[24~");term->s->scrollbottom())

#define KEY(i) (r == OK && (i) == k)
#define CODE(i) (r == KEY_CODE_YES && (i) == k)
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

    void handleUserInput(const std::shared_ptr<NODE> &n,
                         const std::unique_ptr<CursesTerm> &term, int r,
                         wint_t k)
    {
        if (r == KEY_CODE_YES && k == KEY_RESIZE)
        {
            global::reshape(0, 0, LINES, COLS);
            term->s->scrollbottom();
            cmd = false;
            return;
        }

        if (cmd)
        {
            if (r == OK)
            {
                auto found = m_cmdOkMap.find(k);
                if (found != m_cmdOkMap.end())
                {
                    found->second(n, term);
                    cmd = false;
                    return;
                }
            }
            else if (r == KEY_CODE_YES)
            {
                auto found = m_cmdKeyCodeMap.find(k);
                if (found != m_cmdKeyCodeMap.end())
                {
                    found->second(n, term);
                    cmd = false;
                    return;
                }

                if (k == g_commandkey)
                {
                    const char cmdstr[] = {(char)g_commandkey, 0};
                    term->safewrite(cmdstr, 1);
                    cmd = false;
                    return;
                }
            }
            else
            {
                throw std::exception();
            }
        }
        else
        {
            if (r == OK)
            {
                if (k == global::get_commandKey())
                {
                    cmd = true;
                    return;
                }

                auto found = m_okMap.find(k);
                if (found != m_okMap.end())
                {
                    found->second(n, term);
                    cmd = false;
                    return;
                }
            }
            else if (r == KEY_CODE_YES)
            {
                auto found = m_keyCodeMap.find(k);
                if (found != m_keyCodeMap.end())
                {
                    found->second(n, term);
                    cmd = false;
                    return;
                }
            }
            else
            {
                throw std::exception();
            }
        }

        char c[MB_LEN_MAX + 1] = {0};
        if (wctomb(c, k) > 0)
        {
            term->s->scrollbottom();
            term->safewrite(c);
        }
        cmd = false;
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
                        auto &term = focus->term;
                        wint_t k = 0;
                        int r = wget_wch(term->s->win, &k);
                        if (r == ERR)
                        {
                            break;
                        }

                        handleUserInput(focus, term, r, k);
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
}; // namespace global
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
