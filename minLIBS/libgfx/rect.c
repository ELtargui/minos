#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <min/gfx.h>

static int min(int a, int b)
{
    return a < b ? a : b;
}

static int max(int a, int b)
{
    return a > b ? a : b;
}

static inline int r_left(rect_t r)
{
    return r.x;
}

static inline int r_right(rect_t r)
{
    return r.x + r.w;
}

static inline int r_top(rect_t r)
{
    return r.y;
}

static inline int r_bottom(rect_t r)
{
    return r.y + r.h;
}

rect_t Rectangle(int x, int y, int w, int h)
{
    rect_t r = {x, y, w, h};
    return r;
}

rect_t *new_rect(int x, int y, int w, int h)
{
    rect_t *r = malloc(sizeof(rect_t));
    r->x = x;
    r->y = y;
    r->w = w;
    r->h = h;

    return r;
}

int rect_intersects(rect_t r1, rect_t r2)
{
    if (r_left(r1) <= r_right(r2) &&
        r_left(r2) <= r_right(r1) &&
        r_top(r1) <= r_bottom(r2) &&
        r_top(r2) <= r_bottom(r1))
    {
        return 1;
    }

    return 0;
}

rect_t rect_get_intersection(rect_t r1, rect_t r2)
{
    if (!rect_intersects(r1, r2))
    {
        return Rectangle(-1, -1, -1, -1);
    }
    int l = max(r_left(r1), r_left(r2));
    int r = min(r_right(r1), r_right(r2));
    int t = max(r_top(r1), r_top(r2));
    int b = min(r_bottom(r1), r_bottom(r2));

    if (l > r || t > b)
    {
        return Rectangle(-1, -1, -1, -1);
    }

    return Rectangle(l, t, (r - l), (b - t));
}

int point_in_rect(int x, int y, rect_t r)
{
    if (x >= r_left(r) &&
        x < r_right(r) &&
        y >= r_top(r) &&
        y < r_bottom(r))
    {
        return 0;
    }
    return -1;
}

rect_t rect_chrenk(rect_t r, int top, int bottom, int left, int right)
{
    rect_t rect = Rectangle(r.x + left, r.y + bottom, r.w - (right + left), r.h - (bottom + top));

    return rect;
}
