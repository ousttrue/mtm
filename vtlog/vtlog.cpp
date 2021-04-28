#include "config.h"
#include "app.h"
#include "curses_term.h"
#include "node.h"
#include "scrn.h"
#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/FuncMessageFormatter.h>
#include <list>

namespace plog
{

template <class Formatter> class VtAppender : public IAppender
{
    std::function<void(const char *)> m_callback;

    void write(const Record &record) override // This is a method from IAppender
                                              // that MUST be implemented.
    {
        util::nstring str = Formatter::format(
            record); // Use the formatter to get a string from a record.

        if (m_callback)
        {
            m_callback(str.c_str());
        }
    }

public:
    void callback(const std::function<void(const char *msg)> &cb)
    {
        m_callback = cb;
    }
};

} // namespace plog

class VtLogger : public Content
{
    std::unique_ptr<SCRN> m_s;
    int m_count=0;

public:
    VtLogger(const Rect &rect) : Content(rect)
    {
        this->m_s = std::make_unique<SCRN>(SCROLLBACK, rect.w);
    }

    void reshape(int d, int ow, const Rect &rect) override
    {
        m_rect = rect;
        this->m_s->resize(SCROLLBACK, rect.w);
    }

    void draw(const Rect &rect, bool focus) override
    {
        m_rect = rect;
        this->m_s->refresh(m_count-LINES, 0, m_rect.y, m_rect.x, m_rect.y + m_rect.h - 1,
                           m_rect.x + m_rect.w - 1);
    }

    bool handleUserInput(class NODE *node) override
    {
        return true;
    }

    bool process() override
    {
        return true;
    }

    void message(const char *msg)
    {
        this->m_s->add(msg);
        ++m_count;
    }
};

int main(int argc, char **argv)
{
    plog::VtAppender<plog::FuncMessageFormatter> appender;
    plog::init(plog::debug,
               &appender); // Initialize the logger with our appender.

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

    auto logger = std::make_unique<VtLogger>(l->rect());
    appender.callback([l = logger.get()](const char *msg) { l->message(msg); });

    l->term(std::move(logger));
    r->term(CursesTerm::create(r->rect()));

    return App::instance().run(root, r);
}
