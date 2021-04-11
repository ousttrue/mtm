#include "selector.h"
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>

static int g_nfds = 1; /* stdin */

class SelectorImpl
{
    fd_set m_fds;
    fd_set m_sfds;

public:
    SelectorImpl()
    {
        FD_SET(STDIN_FILENO, &m_fds);
    }

    void Set(int fd)
    {
        FD_SET(fd, &m_fds);
    }

    void Close(int fd)
    {
        close(fd);
        FD_CLR(fd, &m_fds);
    }

    void Select(int nfds)
    {
        m_sfds = m_fds;
        if (select(nfds + 1, &m_sfds, NULL, NULL, NULL) < 0)
        {
            FD_ZERO(&m_sfds);
        }
    }

    bool IsSet(int fd)
    {
        return FD_ISSET(fd, &m_sfds);
    }
};
SelectorImpl g_impl;

namespace selector
{

void set(int fd)
{
    g_impl.Set(fd);
    fcntl(fd, F_SETFL, O_NONBLOCK);
    if (fd > g_nfds)
    {
        g_nfds = fd;
    }
}

void close(int fd)
{
    g_impl.Close(fd);
}

void select()
{
    g_impl.Select(g_nfds);
}

bool isSet(int fd)
{
    return g_impl.IsSet(fd);
}

} // namespace selector
