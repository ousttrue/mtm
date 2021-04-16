#pragma once
#include <memory>

bool handlechar(int r, int k); /* Handle a single input character. */
void vp_initialize(const std::unique_ptr<class VtParser> &vp, void *p);
