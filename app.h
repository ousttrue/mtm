#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include "rect.h"

class NODE;
class CursesTerm;
struct CallbackContext
{
    NODE *node;
    CursesTerm *term;
};

using KeyCallbackFunc = std::function<void(const CallbackContext &c)>;

class App
{
    std::shared_ptr<NODE> m_root;
    std::weak_ptr<NODE> m_focused;
    std::weak_ptr<NODE> m_lastfocused;

    std::unordered_map<wint_t, KeyCallbackFunc> m_keyCodeMap;
    std::unordered_map<wint_t, KeyCallbackFunc> m_okMap;
    std::unordered_map<wint_t, KeyCallbackFunc> m_cmdKeyCodeMap;
    std::unordered_map<wint_t, KeyCallbackFunc> m_cmdOkMap;
    bool cmd = false;

    App(const App &) = delete;
    App &operator=(const App &) = delete;
    App();
    ~App();

public:
    void quit();
    void root(const std::shared_ptr<NODE> &node);
    std::shared_ptr<NODE> root() const;
    void focus(const std::shared_ptr<NODE> &n);
    void focus(YX yx);
    std::shared_ptr<NODE> focus() const;
    void focus_last();

private:
    void _handleUserInput(const CallbackContext &c, int r, wint_t k);

public:
    bool handleUserInput(const CallbackContext &c);
    int run();

    static App &instance();

    static void set_term(const char *term);
    static const char *get_term(void);

    static void set_commandkey(int k);
    static int get_commandKey();
};
