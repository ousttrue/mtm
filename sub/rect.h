#pragma once
#include <tuple>

struct YX
{
    int y = 0;
    int x = 0;
};

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

    bool contains(const YX &p) const
    {
        return (p.y >= this->y && p.y <= this->y + this->h && p.x >= this->x &&
                p.x <= this->x + this->w);
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

    YX above() const
    {
        return {y - 2, x + w / 2};
    }

    YX below() const
    {
        return {y + h + 2, x + w / 2};
    }

    YX left() const
    {
        return {y + h / 2, x - 2};
    }

    YX right() const
    {
        return {y + h / 2, x + w + 2};
    }
};
