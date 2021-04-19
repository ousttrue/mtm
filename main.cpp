#include "mtm.h"
#include "vthandler.h"
#include "app.h"
#include <signal.h>
#include <unistd.h>

auto USAGE = "usage: mtm [-T NAME] [-t NAME] [-c KEY]\n";

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    signal(SIGCHLD, SIG_IGN); /* automatically reap children */

    int commandkey = CTL('g');
    const char *term = nullptr;
    int c = 0;
    while ((c = getopt(argc, argv, "c:T:t:")) != -1)
        switch (c)
        {
        case 'c':
            commandkey = CTL(optarg[0]);
            break;
        case 'T':
            setenv("TERM", optarg, 1);
            break;
        case 't':
            term = optarg;
            break;
        default:
            fprintf(stderr, "%s\n", USAGE);
            return EXIT_FAILURE;
        }

    global::set_term(term);
    global::set_commandkey(commandkey);
    auto app = mtm::create();
    return app->run();
}
