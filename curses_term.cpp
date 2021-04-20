#include "curses_term.h"
#include "scrn.h"
#include "minmax.h"
#include "selector.h"
#include "app.h"
#include <climits>
#include <cstring>
#include <curses.h>
#include <memory>
#include <pwd.h>
#include <unistd.h>
#include <signal.h>
#include "vtparser.h"
#include "config.h"
#include "vthandler.h"

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
    this->decom = false;
    this->lnm = false;
    this->s->reset();
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

static int fork_setup(int *pt, const Rect &rect)
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

std::unique_ptr<CursesTerm> new_term(const Rect &rect) /* Open a new view. */
{
    auto term = std::make_unique<CursesTerm>(rect);

    vp_initialize(term);
    auto pid = fork_setup(&term->pt, rect);
    if (pid < 0)
    {
        // error
        perror("forkpty");
        return nullptr;
    }
    else if (pid == 0)
    {
        // child process. not reach here
        return NULL;
    }
    else
    {
        // setup selector
        selector::set(term->pt);
        return term;
    }
}
