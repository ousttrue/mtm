#pragma once
#include <vector>
#include <memory>
#include "rect.h"

struct SCRN;
class VtParser;
struct CursesTerm
{
    Rect m_rect;

private:
    int pt = -1;
    std::vector<bool> m_tabs;

public:
    // Numeric Keypad Mode    
    bool pnm = false;
    
    bool decom = false;
    bool am = false;
    bool lnm = false;
    wchar_t repc = 0;
    std::shared_ptr<SCRN> pri;
    std::shared_ptr<SCRN> alt;
    std::shared_ptr<SCRN> s;
    wchar_t *g0 = nullptr;
    wchar_t *g1 = nullptr;
    wchar_t *g2 = nullptr;
    wchar_t *g3 = nullptr;
    wchar_t *gc = nullptr;
    wchar_t *gs = nullptr;
    wchar_t *sgc = nullptr;
    wchar_t *sgs = nullptr;
    std::unique_ptr<VtParser> vp;

public:
    CursesTerm(const Rect &rect);
    ~CursesTerm();
    static CursesTerm *create(const Rect &rect);
    void reshapeview(int d, int ow, const Rect &rect);
    void draw(const Rect &rect, bool focus);
    bool process();
    //
    void scrollback();
    void scrollforward();
    // SENDN(n, s, c) - Write string c bytes of s to n.
    void safewrite(const char *b, size_t n); /* Write, checking for errors. */
    // safewrite( s)     - Write string s to node n's host.
    void safewrite(const char *s);
    void sendarrow(const char *k);
    void reset();
    bool alternate_screen_buffer_mode(bool set);
    bool TryGetBackwardTab(int x, int *out);
    bool TryGetForwardTab(int x, int *out);
    void TabClear(int x);
    void TabClearAll();
    void HorizontalTabSet(int x);

private:
    bool handleUserInput();
    void fixCursor();
    bool INSCR() const;
};
