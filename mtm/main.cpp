#include "app.h"
#include "node.h"
#include "curses_term.h"
#include "scrn.h"
#include <unistd.h>
#include <curses.h>

auto USAGE = "usage: mtm [-T NAME] [-t NAME] [-c KEY]\n";

int main(int argc, char **argv)
{
    if(!App::initialize())
    {
        return 1;
    }

    int commandkey = 'g';
    const char *term = nullptr;
    int c = 0;
    while ((c = getopt(argc, argv, "c:T:t:")) != -1)
        switch (c)
        {
        case 'c':
            commandkey = optarg[0];
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

    App::set_commandkey(commandkey);

    //
    // default layout
    //
    auto root = std::make_shared<NODE>(App::create_term(term));

    //
    // key binding
    //
    App::instance().keyCodeMap(true, KEY_UP, [](const CallbackContext &c) {
        App::instance().focus(c.term->rect().above());
    });
    App::instance().keyCodeMap(true, KEY_DOWN, [](const CallbackContext &c) {
        App::instance().focus(c.term->rect().below());
    });
    App::instance().keyCodeMap(true, KEY_LEFT, [](const CallbackContext &c) {
        App::instance().focus(c.term->rect().left());
    });
    App::instance().keyCodeMap(true, KEY_RIGHT, [](const CallbackContext &c) {
        App::instance().focus(c.term->rect().right());
    });

    App::instance().keyCodeMap(true, KEY_PPAGE, [](const CallbackContext &c) {
        c.term->scrollback();
    });
    App::instance().keyCodeMap(true, KEY_NPAGE, [](const CallbackContext &c) {
        c.term->scrollforward();
    });
    App::instance().keyCodeMap(true, KEY_END, [](const CallbackContext &c) {
        c.term->s->scrollbottom();
    });

    App::instance().okMap(true, L'o', [](const CallbackContext &c) {
        App::instance().focus_last();
    });
    App::instance().okMap(
        true, L'h', [](const CallbackContext &c) { c.node->split(true); });
    App::instance().okMap(
        true, L'v', [](const CallbackContext &c) { c.node->split(false); });
    App::instance().okMap(true, L'w',
                          [](const CallbackContext &c) { c.node->close(); });
    App::instance().okMap(true, L'l', [](const CallbackContext &c) {
        touchwin(stdscr);
        redrawwin(stdscr);
    });

    App::instance().okMap(false, 0, [](const CallbackContext &c) {
        c.term->safewrite("\000", 1);
        c.term->s->scrollbottom();
    });
    App::instance().okMap(false, L'\n', [](const CallbackContext &c) {
        c.term->safewrite("\n");
        c.term->s->scrollbottom();
    });
    App::instance().okMap(false, L'\r', [](const CallbackContext &c) {
        c.term->safewrite(c.term->lnm ? "\r\n" : "\r");
        c.term->s->scrollbottom();
    });

    App::instance().keyCodeMap(false, KEY_PPAGE, [](const CallbackContext &c) {
        if (c.term->s->INSCR())
        {
            c.term->scrollback();
        }
    });
    App::instance().keyCodeMap(false, KEY_NPAGE, [](const CallbackContext &c) {
        if (c.term->s->INSCR())
        {
            c.term->scrollforward();
        }
    });
    App::instance().keyCodeMap(false, KEY_END, [](const CallbackContext &c) {
        if (c.term->s->INSCR())
        {
            c.term->s->scrollbottom();
        }
    });

    App::instance().keyCodeMap(false, KEY_ENTER, [](const CallbackContext &c) {
        c.term->safewrite(c.term->lnm ? "\r\n" : "\r");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_UP, [](const CallbackContext &c) {
        c.term->sendarrow("A");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_DOWN, [](const CallbackContext &c) {
        c.term->sendarrow("B");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_RIGHT, [](const CallbackContext &c) {
        c.term->sendarrow("C");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_LEFT, [](const CallbackContext &c) {
        c.term->sendarrow("D");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_HOME, [](const CallbackContext &c) {
        c.term->safewrite("\033[1~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_END, [](const CallbackContext &c) {
        c.term->safewrite("\033[4~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_PPAGE, [](const CallbackContext &c) {
        c.term->safewrite("\033[5~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_NPAGE, [](const CallbackContext &c) {
        c.term->safewrite("\033[6~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_BACKSPACE,
                               [](const CallbackContext &c) {
                                   c.term->safewrite("\177");
                                   c.term->s->scrollbottom();
                               });
    App::instance().keyCodeMap(false, KEY_DC, [](const CallbackContext &c) {
        c.term->safewrite("\033[3~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_IC, [](const CallbackContext &c) {
        c.term->safewrite("\033[2~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_BTAB, [](const CallbackContext &c) {
        c.term->safewrite("\033[Z");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(1), [](const CallbackContext &c) {
        c.term->safewrite("\033OP");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(2), [](const CallbackContext &c) {
        c.term->safewrite("\033OQ");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(3), [](const CallbackContext &c) {
        c.term->safewrite("\033OR");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(4), [](const CallbackContext &c) {
        c.term->safewrite("\033OS");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(5), [](const CallbackContext &c) {
        c.term->safewrite("\033[15~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(6), [](const CallbackContext &c) {
        c.term->safewrite("\033[17~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(7), [](const CallbackContext &c) {
        c.term->safewrite("\033[18~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(8), [](const CallbackContext &c) {
        c.term->safewrite("\033[19~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(9), [](const CallbackContext &c) {
        c.term->safewrite("\033[20~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(10), [](const CallbackContext &c) {
        c.term->safewrite("\033[21~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(11), [](const CallbackContext &c) {
        c.term->safewrite("\033[23~");
        c.term->s->scrollbottom();
    });
    App::instance().keyCodeMap(false, KEY_F(12), [](const CallbackContext &c) {
        c.term->safewrite("\033[24~");
        c.term->s->scrollbottom();
    });

    return App::instance().run(root);
}
