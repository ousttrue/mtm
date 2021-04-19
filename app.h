#pragma once
#include <memory>
#include "rect.h"

inline int CTL(int x)
{
    return ((x)&0x1f);
}

class NODE;
namespace global
{

void set_term(const char *term);
const char *get_term(void);

void set_commandkey(int k);
int get_commandKey();

// focus
std::shared_ptr<NODE> focus();
void focus(const std::shared_ptr<NODE> &node);
void focus(YX yx);
void focus_last();
int run();

// root
void quit();
std::shared_ptr<NODE> root();
void root(const std::shared_ptr<NODE> &node);
void reshape(int y, int x, int h, int w);
void draw();

} // namespace global
