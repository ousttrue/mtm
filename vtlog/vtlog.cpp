#include "app.h"
#include "curses_term.h"
#include "node.h"
#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/FuncMessageFormatter.h>
#include <list>

namespace plog
{
template <class Formatter> // Typically a formatter is passed as a template
                           // parameter.
class MyAppender
    : public IAppender // All appenders MUST inherit IAppender interface.
{
public:
    virtual void write(const Record &record) // This is a method from IAppender
                                             // that MUST be implemented.
    {
        util::nstring str = Formatter::format(
            record); // Use the formatter to get a string from a record.

        m_messageList.push_back(str); // Store a log message in a list.
    }

    std::list<util::nstring> &getMessageList()
    {
        return m_messageList;
    }

private:
    std::list<util::nstring> m_messageList;
};
} // namespace plog

class Logger : public Content
{
public:
    Logger(const Rect &rect) : Content(rect)
    {
    }

    void reshape(int d, int ow, const Rect &rect) override
    {
    }
    void draw(const Rect &rect, bool focus) override
    {
    }
    bool handleUserInput(class NODE *node) override
    {
        return true;
    }
    bool process() override
    {
        return true;
    }
};

int main(int argc, char **argv)
{
    static plog::MyAppender<plog::FuncMessageFormatter>
        myAppender; // Create our custom appender.
    plog::init(plog::debug,
               &myAppender); // Initialize the logger with our appender.

    // Step3: write log messages using a special macro
    // There are several log macros, use the macro you liked the most

    PLOGD << "Hello log!";             // short macro
    PLOG_DEBUG << "Hello log!";        // long macro
    PLOG(plog::debug) << "Hello log!"; // function-style macro

    // Also you can use LOG_XXX macro but it may clash with other logging
    // libraries
    LOGD << "Hello log!";             // short macro
    LOG_DEBUG << "Hello log!";        // long macro
    LOG(plog::debug) << "Hello log!"; // function-style macro

    App::initialize();
    //
    // default layout
    //
    auto root = std::make_shared<NODE>();
    std::shared_ptr<NODE> l;
    std::shared_ptr<NODE> r;
    std::tie(l, r) = root->splitHorizontal();

    l->term(new Logger(l->rect()));
    // l->term(CursesTerm::create(l->rect()));

    r->term(CursesTerm::create(r->rect()));

    return App::instance().run(root, r);
}
