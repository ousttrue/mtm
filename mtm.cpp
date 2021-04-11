/* Copyright 2017 - 2019 Rob King <jking@deadpixi.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <memory>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "selector.h"
#include "minmax.h"
#include "mtm.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "vt/vtparser.h"
#include "pair.h"

#ifdef __cplusplus
}
#endif

#include "vthandler.h"
#include "node.h"
#include "scrn.h"


/*** MTM FUNCTIONS
 * These functions do the user-visible work of MTM: creating nodes in the
 * tree, updating the display, and so on.
 */
static bool getinput(const std::shared_ptr<NODE>
                         &n) /* Recursively check all ptty's for input. */
{
    if (n && n->c1 && !getinput(n->c1))
        return false;

    if (n && n->c2 && !getinput(n->c2))
        return false;

    if (n && n->isView() && n->pt > 0 && selector::isSet(n->pt))
    {
        char g_iobuf[BUFSIZ];
        ssize_t r = read(n->pt, g_iobuf, sizeof(g_iobuf));
        if (r > 0)
        {
            vtwrite(&n->vp, g_iobuf, r);
        }
        if (r <= 0 && errno != EINTR && errno != EWOULDBLOCK)
        {
            deletenode(n);
            return false;
        }
    }

    return true;
}

mtm::mtm()
{
    raw();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    start_color();
    use_default_colors();
    start_pairs();

    run();
}

std::unique_ptr<mtm> mtm::create(const char *term, int commandKey)
{
    set_term(term);
    set_commandkey(commandKey);
    if (!initscr())
    {
        // quit(EXIT_FAILURE, "could not initialize terminal");
        return nullptr;
    }

    return std::unique_ptr<mtm>(new mtm());
}

mtm::~mtm()
{
    // void quit(int rc, const char *m) /* Shut down MTM. */
    // {
    //     if (m)
    //         fprintf(stderr, "%s\n", m);
    root = nullptr;
    endwin();
    // exit(rc);
}

int mtm::run()
{
    root = newview(NULL, 0, 0, LINES, COLS);
    if (!root)
    {
        return 1;
        // quit(EXIT_FAILURE, "could not open root window");
    }
    focus(root);
    root->draw();

    while (root)
    {
        wint_t w = 0;
        selector::select();

        {
            auto f = focused.lock();
            int r = wget_wch(f->s->win, &w);
            while (handlechar(r, w))
            {
                r = wget_wch(f->s->win, &w);
            }
        }

        if (!getinput(root))
        {
            break;
        }

        root->draw();
        doupdate();
        {
            auto f = focused.lock();
            if (f)
            {
                f->s->fixcursor(f->h);
            }
            f->draw();
        }
        doupdate();
    }

    return EXIT_SUCCESS;
}
