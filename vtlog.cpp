#include "app.h"
#include "curses_term.h"
#include "node.h"
#include <signal.h>
#include <unistd.h>
#include <curses.h>

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    signal(SIGCHLD, SIG_IGN); /* automatically reap children */
    App::initialize(nullptr, 'g');

    //
    // default layout
    //
    auto root = std::make_shared<NODE>(CursesTerm::create());
    App::instance().root(root);

    return App::instance().run();
}
