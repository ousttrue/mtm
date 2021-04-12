#pragma once
#include <vector>
#include <memory>

struct SCRN;
struct VTPARSER;
struct VTScreen
{
    int pt = -1;

    std::vector<bool> tabs;
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
    std::unique_ptr<VTPARSER> vp;

    VTScreen(int lines, int cols);
    ~VTScreen();
    void reshapeview(int d, int ow, int lines, int cols);
    void draw(int y, int x, int h, int w);
    bool process();
    bool handleUserInput();
    void fixCursor(int h);
    void reset(int h);
    bool alternate_screen_buffer_mode(bool set);
};
