#include "mtm.h"
#include <clocale>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

auto USAGE = "usage: mtm [-T NAME] [-t NAME] [-c KEY]\n";

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    signal(SIGCHLD, SIG_IGN); /* automatically reap children */

    int commandkey = 'g';
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
            quit(EXIT_FAILURE, USAGE);
            break;
        }

    return mtm(term, commandkey);
}
