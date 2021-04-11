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
#include <cstdio>
#include <unistd.h>
#include "selector.h"
#include "mtm.h"
#include "vthandler.h"
#include "scrn.h"
#include "node.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "pair.h"

#ifdef __cplusplus
}
#endif

mtm::mtm()
{
    raw();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    start_color();
    use_default_colors();
    start_pairs();
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
    root = nullptr;
    endwin();
}

int mtm::run()
{
    root = newview(NULL, 0, 0, LINES, COLS);
    if (!root)
    {
        return 1;
    }
    focus(root);

    //
    // main loop
    //
    for (int i = 0; true; ++i)
    {
        //
        // process all user input
        //
        {
            auto f = focused.lock();
            while (true)
            {
                wint_t w = 0;
                int r = wget_wch(f->s->win, &w);
                if (!handlechar(r, w))
                {
                    break;
                }
            }
        }

        //
        // read pty and process vt
        //
        selector::select();
        root->processVT();
        if (!root)
        {
            break;
        }

        //
        // update visual
        //
        {
            root->draw();

            // cursor for focused
            auto f = focused.lock();
            if (f)
            {
                f->s->fixcursor(f->h);
                f->draw();
            }

            doupdate();
        }
    }

    return EXIT_SUCCESS;
}
