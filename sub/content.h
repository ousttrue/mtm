#pragma once
#include "rect.h"

class Content
{
protected:
    Rect m_rect = {};
    Content(const Rect &rect) : m_rect(rect)
    {
    }

public:
    virtual ~Content()
    {
    }

    Rect rect() const
    {
        return m_rect;
    }

    virtual void reshape(int d, int ow, const Rect &rect) = 0;
    virtual void draw(const Rect &rect, bool focus) = 0;
    virtual bool handleUserInput(class NODE *node) = 0;
    virtual bool process() = 0;
};
