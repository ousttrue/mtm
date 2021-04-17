#include "curses_term.h"
#include "scrn.h"
#include "minmax.h"
#include "selector.h"
#include "vthandler.h"
#include "global.h"
#include <climits>
#include <cstring>
#include <curses.h>
#include <memory>
#include <pwd.h>
#include <unistd.h>
#include <signal.h>
#include "vtparser.h"
#include "config.h"

CursesTerm::CursesTerm(const Rect &rect) : m_rect(rect)
{
    for (int i = 0; i < m_rect.w; i++)
    {
        /* keep old overlapping m_tabs */
        m_tabs.push_back(i % 8 == 0);
    }

    this->pri = std::make_shared<SCRN>();
    this->alt = std::make_shared<SCRN>();

    this->pri->win = newpad(MAX(m_rect.h, SCROLLBACK), m_rect.w);
    this->alt->win = newpad(m_rect.h, m_rect.w);
    if (!pri->win || !alt->win)
    {
        throw "new pad";
        // return nullptr;
    }

    pri->tos = pri->off = MAX(0, SCROLLBACK - m_rect.h);
    this->s = this->pri;

    nodelay(pri->win, TRUE);
    nodelay(alt->win, TRUE);
    scrollok(pri->win, TRUE);
    scrollok(alt->win, TRUE);
    keypad(pri->win, TRUE);
    keypad(alt->win, TRUE);

    this->vp = std::make_unique<VtParser>();
}

CursesTerm::~CursesTerm()
{
    if (this->pri->win)
        delwin(this->pri->win);
    if (this->alt->win)
        delwin(this->alt->win);
    if (this->pt >= 0)
    {
        selector::close(this->pt);
    }
}

void CursesTerm::reshapeview(int d, int ow,
                             const Rect &rect) /* Reshape a view. */
{
    m_rect = rect;
    struct winsize ws = {.ws_row = (unsigned short)m_rect.h,
                         .ws_col = (unsigned short)m_rect.w};

    {
        auto oldtabs = m_tabs;
        this->m_tabs.clear();
        for (int i = 0; i < m_rect.w; i++) /* keep old overlapping m_tabs */
            m_tabs.push_back(i < ow ? oldtabs[i] : (i % 8 == 0));
    }

    int oy, ox;
    getyx(this->s->win, oy, ox);
    wresize(this->pri->win, MAX(m_rect.h, SCROLLBACK), MAX(m_rect.w, 2));
    wresize(this->alt->win, MAX(m_rect.h, 2), MAX(m_rect.w, 2));
    this->pri->tos = this->pri->off = MAX(0, SCROLLBACK - m_rect.h);
    this->alt->tos = this->alt->off = 0;
    wsetscrreg(this->pri->win, 0, MAX(SCROLLBACK, m_rect.h) - 1);
    wsetscrreg(this->alt->win, 0, m_rect.h - 1);
    if (d > 0)
    { /* make sure the new top line syncs up after reshape */
        wmove(this->s->win, oy + d, ox);
        wscrl(this->s->win, -d);
    }
    doupdate();
    refresh();
    ioctl(this->pt, TIOCSWINSZ, &ws);
}

void CursesTerm::draw(const Rect &rect)
{
    m_rect = rect;
    pnoutrefresh(this->s->win, this->s->off, 0, m_rect.y, m_rect.x,
                 m_rect.y + m_rect.h - 1, m_rect.x + m_rect.w - 1);
}

bool CursesTerm::process()
{
    if (this->pt > 0 && selector::isSet(this->pt))
    {
        char g_iobuf[BUFSIZ];
        ssize_t r = read(this->pt, g_iobuf, sizeof(g_iobuf));
        if (r > 0)
        {
            this->vp->write(this, g_iobuf, r);
        }
        if (r <= 0 && errno != EINTR && errno != EWOULDBLOCK)
        {
            return false;
        }
    }

    return true;
}

#include "node.h"

void sendarrow(const std::shared_ptr<NODE> &n, const char *k)
{
    char buf[100] = {0};
    snprintf(buf, sizeof(buf) - 1, "\033%s%s", n->term->pnm ? "O" : "[", k);
    n->term->safewrite(buf);
}

bool CursesTerm::handleUserInput()
{
    wint_t k = 0;
    int r = wget_wch(this->s->win, &k);

    // return handlechar(r, w);
    const char cmdstr[] = {(char)global::get_commandKey(), 0};
    static bool cmd = false;
    auto n = focused.lock();
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
    DO(cmd, CODE(KEY_RESIZE), root->reshape(Rect(0, 0, LINES, COLS)); SB)
    DO(false, KEY(global::get_commandKey()), return cmd = true)
    DO(false, KEY(0), n->term->safewrite("\000", 1); SB)
    DO(false, KEY(L'\n'), n->term->safewrite("\n"); SB)
    DO(false, KEY(L'\r'), n->term->safewrite(n->term->lnm ? "\r\n" : "\r"); SB)
    DO(false, SCROLLUP && INSCR, n->term->s->scrollback(n->m_rect.h))
    DO(false, SCROLLDOWN && INSCR, n->term->s->scrollforward(n->m_rect.h))
    DO(false, RECENTER && INSCR, n->term->s->scrollbottom())
    DO(false, CODE(KEY_ENTER), n->term->safewrite(n->term->lnm ? "\r\n" : "\r");
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
    DO(true, MOVE_UP, focus(root->findnode(n->m_rect.above())))
    DO(true, MOVE_DOWN, focus(root->findnode(n->m_rect.below())))
    DO(true, MOVE_LEFT, focus(root->findnode(n->m_rect.left())))
    DO(true, MOVE_RIGHT, focus(root->findnode(n->m_rect.right())))
    DO(true, MOVE_OTHER, focus(lastfocused.lock()))
    DO(true, HSPLIT, split(n, HORIZONTAL))
    DO(true, VSPLIT, split(n, VERTICAL))
    DO(true, DELETE_NODE, deletenode(n))
    DO(true, REDRAW, touchwin(stdscr); root->draw(); redrawwin(stdscr))
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

void CursesTerm::fixCursor()
{
    this->s->fixcursor(m_rect.h);
}

void CursesTerm::reset()
{
    this->gs = this->gc = this->g0 = CSET_US;
    this->g1 = CSET_GRAPH;
    this->g2 = CSET_US;
    this->g3 = CSET_GRAPH;
    this->decom = s->insert = s->oxenl = s->xenl = this->lnm = false;
    this->am = true;
    this->pnm = false;
    this->pri->vis = this->alt->vis = 1;
    this->s = this->pri;
    wsetscrreg(this->pri->win, 0, MAX(SCROLLBACK, m_rect.h) - 1);
    wsetscrreg(this->alt->win, 0, m_rect.h - 1);
    for (int i = 0; i < this->m_tabs.size(); i++)
        this->m_tabs[i] = (i % 8 == 0);
}

bool CursesTerm::alternate_screen_buffer_mode(bool set)
{
    if (set && this->s != this->alt)
    {
        this->s = this->alt;
        return true;
        // CALL(cls);
    }
    else if (!set && this->s != this->pri)
    {
        this->s = this->pri;
    }
    return false;
}

static const char *getshell(void) /* Get the user's preferred shell. */
{
    if (getenv("SHELL"))
        return getenv("SHELL");
    struct passwd *pwd = getpwuid(getuid());
    if (pwd)
        return pwd->pw_shell;
    return "/bin/sh";
}

int fork_setup(int *pt, const Rect &rect)
{
    struct winsize ws = {.ws_row = (unsigned short)rect.h,
                         .ws_col = (unsigned short)rect.w};
    pid_t pid = forkpty(pt, NULL, NULL, &ws);
    if (pid == 0)
    {
        //
        // new process
        //
        char buf[100] = {0};
        snprintf(buf, sizeof(buf) - 1, "%lu", (unsigned long)getppid());
        setsid();
        setenv("MTM", buf, 1);
        setenv("TERM", global::get_term(), 1);
        signal(SIGCHLD, SIG_DFL);
        execl(getshell(), getshell(), NULL);

        // not reach here
    }
    return pid;
}

void CursesTerm::HorizontalTabSet(int x)
{
    if (x < m_tabs.size() && x > 0)
        m_tabs[x] = true;
}

bool CursesTerm::TryGetBackwardTab(int x, int *out)
{
    for (int i = x - 1; i < m_tabs.size() && i >= 0; i--)
        if (m_tabs[i])
        {
            *out = i;
            return true;
        }

    return false;
}

bool CursesTerm::TryGetForwardTab(int x, int *out)
{
    for (int i = x + 1; i < m_rect.w && i < m_tabs.size(); i++)
        if (m_tabs[i])
        {
            *out = i;
            return true;
        }

    return false;
}

void CursesTerm::TabClear(int x)
{
    m_tabs[x < m_tabs.size() ? x : 0] = false;
}

void CursesTerm::TabClearAll()
{
    for (int i = 0; i < m_tabs.size(); ++i)
    {
        m_tabs[i] = false;
    }
}

void CursesTerm::safewrite(const char *b,
                           size_t n) /* Write, checking for errors. */
{
    size_t w = 0;
    while (w < n)
    {
        ssize_t s = write(pt, b + w, n - w);
        if (s < 0 && errno != EINTR)
            return;
        else if (s < 0)
            s = 0;
        w += (size_t)s;
    }
}

void CursesTerm::safewrite(const char *s)
{
    safewrite(s, strlen(s));
}
