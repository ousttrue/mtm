#include "app.h"
#include "curses_term.h"
#include "node.h"

int main(int argc, char **argv)
{
    //
    // default layout
    //
    auto root = std::make_shared<NODE>(App::create_term());

    return App::instance().run(root);
}
