#pragma once
#include "curses_term.h"
#include <memory>

void vp_initialize(const std::unique_ptr<struct CursesTerm> &p);
