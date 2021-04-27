#include "app.h"
#include "node.h"
#include "curses_term.h"
#include "scrn.h"
#include "selector.h"
#include "pair.h"
#include <climits>
#include <cstring>
#include <exception>
#include <string>
#include <unordered_map>

//
// Node manipulation
//

App::App()
{
    if (!initscr())
    {
        // quit(EXIT_FAILURE, "could not initialize terminal");
        throw std::exception();
    }

    raw();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    start_color();
    use_default_colors();
    start_pairs();

    auto rect = Rect(0, 0, LINES, COLS);
    m_root = std::make_shared<NODE>(rect, CursesTerm::create(rect));
    focus(m_root);

    //
    // cmd == true
    //
    m_cmdKeyCodeMap.insert({KEY_UP, [self = this](const CallbackContext &c) {
                                self->focus(c.term->m_rect.above());
                            }});
    m_cmdKeyCodeMap.insert({KEY_DOWN, [self = this](const CallbackContext &c) {
                                self->focus(c.term->m_rect.below());
                            }});
    m_cmdKeyCodeMap.insert({KEY_LEFT, [self = this](const CallbackContext &c) {
                                self->focus(c.term->m_rect.left());
                            }});
    m_cmdKeyCodeMap.insert({KEY_RIGHT, [self = this](const CallbackContext &c) {
                                self->focus(c.term->m_rect.right());
                            }});
    m_cmdKeyCodeMap.insert(
        {KEY_PPAGE, [](const CallbackContext &c) { c.term->scrollback(); }});
    m_cmdKeyCodeMap.insert(
        {KEY_NPAGE, [](const CallbackContext &c) { c.term->scrollforward(); }});
    m_cmdKeyCodeMap.insert(
        {KEY_END, [](const CallbackContext &c) { c.term->s->scrollbottom(); }});

    m_cmdOkMap.insert({L'o', [self = this](const CallbackContext &c) {
                           self->focus_last();
                       }});
    m_cmdOkMap.insert(
        {L'h', [](const CallbackContext &c) { c.node->split(true); }});
    m_cmdOkMap.insert(
        {L'v', [](const CallbackContext &c) { c.node->split(false); }});
    m_cmdOkMap.insert(
        {L'w', [](const CallbackContext &c) { c.node->close(); }});
    m_cmdOkMap.insert({L'l', [](const CallbackContext &c) {
                           touchwin(stdscr);
                           redrawwin(stdscr);
                       }});

    //
    // cmd == false
    //
    m_okMap.insert({0, [](const CallbackContext &c) {
                        c.term->safewrite("\000", 1);
                        c.term->s->scrollbottom();
                    }});
    m_okMap.insert({L'\n', [](const CallbackContext &c) {
                        c.term->safewrite("\n");
                        c.term->s->scrollbottom();
                    }});
    m_okMap.insert({L'\r', [](const CallbackContext &c) {
                        c.term->safewrite(c.term->lnm ? "\r\n" : "\r");
                        c.term->s->scrollbottom();
                    }});

    m_keyCodeMap.insert({KEY_PPAGE, [](const CallbackContext &c) {
                             if (c.term->s->INSCR())
                             {
                                 c.term->scrollback();
                             }
                         }});
    m_keyCodeMap.insert({KEY_NPAGE, [](const CallbackContext &c) {
                             if (c.term->s->INSCR())
                             {
                                 c.term->scrollforward();
                             }
                         }});
    m_keyCodeMap.insert({KEY_END, [](const CallbackContext &c) {
                             if (c.term->s->INSCR())
                             {
                                 c.term->s->scrollbottom();
                             }
                         }});

    m_keyCodeMap.insert({KEY_ENTER, [](const CallbackContext &c) {
                             c.term->safewrite(c.term->lnm ? "\r\n" : "\r");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_UP, [](const CallbackContext &c) {
                             c.term->sendarrow("A");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_DOWN, [](const CallbackContext &c) {
                             c.term->sendarrow("B");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_RIGHT, [](const CallbackContext &c) {
                             c.term->sendarrow("C");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_LEFT, [](const CallbackContext &c) {
                             c.term->sendarrow("D");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_HOME, [](const CallbackContext &c) {
                             c.term->safewrite("\033[1~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_END, [](const CallbackContext &c) {
                             c.term->safewrite("\033[4~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_PPAGE, [](const CallbackContext &c) {
                             c.term->safewrite("\033[5~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_NPAGE, [](const CallbackContext &c) {
                             c.term->safewrite("\033[6~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_BACKSPACE, [](const CallbackContext &c) {
                             c.term->safewrite("\177");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_DC, [](const CallbackContext &c) {
                             c.term->safewrite("\033[3~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_IC, [](const CallbackContext &c) {
                             c.term->safewrite("\033[2~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_BTAB, [](const CallbackContext &c) {
                             c.term->safewrite("\033[Z");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(1), [](const CallbackContext &c) {
                             c.term->safewrite("\033OP");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(2), [](const CallbackContext &c) {
                             c.term->safewrite("\033OQ");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(3), [](const CallbackContext &c) {
                             c.term->safewrite("\033OR");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(4), [](const CallbackContext &c) {
                             c.term->safewrite("\033OS");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(5), [](const CallbackContext &c) {
                             c.term->safewrite("\033[15~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(6), [](const CallbackContext &c) {
                             c.term->safewrite("\033[17~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(7), [](const CallbackContext &c) {
                             c.term->safewrite("\033[18~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(8), [](const CallbackContext &c) {
                             c.term->safewrite("\033[19~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(9), [](const CallbackContext &c) {
                             c.term->safewrite("\033[20~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(10), [](const CallbackContext &c) {
                             c.term->safewrite("\033[21~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(11), [](const CallbackContext &c) {
                             c.term->safewrite("\033[23~");
                             c.term->s->scrollbottom();
                         }});
    m_keyCodeMap.insert({KEY_F(12), [](const CallbackContext &c) {
                             c.term->safewrite("\033[24~");
                             c.term->s->scrollbottom();
                         }});
}

App::~App()
{
    endwin();
}

void App::quit()
{
    m_root = nullptr;
}
void App::root(const std::shared_ptr<NODE> &node)
{
    m_root = node;
}
std::shared_ptr<NODE> App::root() const
{
    return m_root;
}

void App::focus(const std::shared_ptr<NODE> &n) /* Focus a node. */
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
void App::focus(YX yx)
{
    auto found = m_root->findnode(yx);
    focus(found);
}
std::shared_ptr<NODE> App::focus() const
{
    return m_focused.lock();
}
void App::focus_last()
{
    if (auto last = m_lastfocused.lock())
    {
        focus(last);
    }
}

void App::_handleUserInput(const CallbackContext &c, int r, wint_t k)
{
    if (r == KEY_CODE_YES && k == KEY_RESIZE)
    {
        c.term->s->scrollbottom();
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
                found->second(c);
                cmd = false;
                return;
            }
        }
        else if (r == KEY_CODE_YES)
        {
            auto found = m_cmdKeyCodeMap.find(k);
            if (found != m_cmdKeyCodeMap.end())
            {
                found->second(c);
                cmd = false;
                return;
            }

            if (k == get_commandKey())
            {
                const char cmdstr[] = {(char)get_commandKey(), 0};
                c.term->safewrite(cmdstr, 1);
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
            if (k == get_commandKey())
            {
                cmd = true;
                return;
            }

            auto found = m_okMap.find(k);
            if (found != m_okMap.end())
            {
                found->second(c);
                cmd = false;
                return;
            }
        }
        else if (r == KEY_CODE_YES)
        {
            auto found = m_keyCodeMap.find(k);
            if (found != m_keyCodeMap.end())
            {
                found->second(c);
                cmd = false;
                return;
            }
        }
        else
        {
            throw std::exception();
        }
    }

    char buf[MB_LEN_MAX + 1] = {0};
    if (wctomb(buf, k) > 0)
    {
        c.term->s->scrollbottom();
        c.term->safewrite(buf);
    }
    cmd = false;
}

bool App::handleUserInput(const CallbackContext &c)
{
    auto input = c.term->s->input();
    if (input.status == ERR)
    {
        return false;
    }

    _handleUserInput(c, input.status, input.code);
    return true;
}

int App::run()
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
                    if (!handleUserInput({focus.get(), focus->term()}))
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
        if (!m_root->process())
        {
            break;
        }

        //
        // update visual
        //
        {
            // cursor for focused
            auto f = m_focused.lock();
            m_root->draw(f);
            doupdate();
        }
    }

    return EXIT_SUCCESS;
}

App &App::instance()
{
    static App s_instance;
    return s_instance;
}

inline int CTL(int x)
{
    return ((x)&0x1f);
}
/* The default command prefix key, when modified by cntrl.
 * This can be changed at runtime using the '-c' flag.
 */
#define COMMAND_KEY 'g'
std::string g_term;
static int g_commandkey = CTL(COMMAND_KEY);
#define DEFAULT_TERMINAL "screen-bce"
#define DEFAULT_256_COLOR_TERMINAL "screen-256color-bce"

void App::set_term(const char *term)
{
    if (term)
    {
        g_term = term;
    }
}

const char *App::get_term(void)
{
    const char *envterm = getenv("TERM");
    if (!g_term.empty())
        return g_term.c_str();
    if (envterm && COLORS >= 256 && !strstr(DEFAULT_TERMINAL, "-256color"))
        return DEFAULT_256_COLOR_TERMINAL;
    return DEFAULT_TERMINAL;
}

void App::set_commandkey(int k)
{
    g_commandkey = CTL(k);
}

int App::get_commandKey()
{
    return g_commandkey;
}
