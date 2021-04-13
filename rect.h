#pragma once
#include <tuple>

struct Rect
{
    int y = 0;
    int x = 0;
    int h = 0;
    int w = 0;

    Rect()
    {
    }

    Rect(int _y, int _x, int _h, int _w) : y(_y), x(_x), h(_h), w(_w)
    {
    }

    bool operator==(const Rect &rhs) const
    {
        return y == rhs.y && x == rhs.x && h == rhs.h && w == rhs.w;
    }

    bool contains(int y, int x) const
    {
        return (y >= this->y && y <= this->y + this->h && x >= this->x &&
                x <= this->x + this->w);
    }

    std::tuple<Rect, Rect> splitHorizontal() const
    {
        int i = w % 2 ? 0 : 1;
        return std::make_tuple(
            Rect(this->y, this->x, this->h, this->w / 2),
            Rect(this->y, this->x + this->w / 2 + 1, this->h, this->w / 2 - i));
    }

    std::tuple<Rect, Rect> splitVertical() const
    {
        int i = this->h % 2 ? 0 : 1;
        return std::make_tuple(
            Rect(this->y, this->x, this->h / 2, this->w),
            Rect(this->y + this->h / 2 + 1, this->x, this->h / 2 - i, this->w));
    }
};
