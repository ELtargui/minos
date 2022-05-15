#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <min/list.h>
#include <min/gfx.h>

static int min(int a, int b)
{
    return a < b ? a : b;
}

static int max(int a, int b)
{
    return a > b ? a : b;
}

surface_t *surface_from_buffer(void *buffer, int w, int h)
{
    surface_t *surface = malloc(sizeof(surface_t));
    surface->w = w;
    surface->h = h;
    surface->buffer = buffer;
    return surface;
}
void surface_setpixel(surface_t *s, int x, int y, color_t c)
{
    if (x > s->w || y > s->h)
        assert(0);
    s->buffer[y * s->w + x] = c.raw;
}

color_t surface_getpixel(surface_t *s, int x, int y)
{
    if (x > s->w || y > s->h)
        assert(0);
    return color_u32(s->buffer[y * s->w + x]);
}
surface_t *new_surface(int w, int h)
{
    void *buffer = calloc(1, w * h * sizeof(color_t));
    assert(buffer);
    return surface_from_buffer(buffer, w, h);
}

void surface_fill(surface_t *s, color_t color)
{
    for (int y = 0; y < s->h; y++)
    {
        for (int x = 0; x < s->w; x++)
        {
            s->buffer[y * s->w + x] = color.raw;
        }
    }
}

void surface_fill_rect(surface_t *s, rect_t rect, color_t color)
{
    for (int y = rect.y; y < rect.y + rect.h; y++)
    {
        for (int x = rect.x; x < rect.x + rect.w; x++)
        {
            s->buffer[y * s->w + x] = color.raw;
        }
    }
}

void surface_draw_line(surface_t *surface, int x, int y, int xend, int yend, int theckness, color_t color)
{
    if (x == xend)
    {
        //vertical line
        for (int j = y; j < yend; j++)
        {
            for (int i = x; i < x + theckness; i++)
            {
                surface_setpixel(surface, i, j, color);
            }
        }
    }
    else if (y == yend)
    {
        //horizontal line
        for (int j = y; j < y + theckness; j++)
        {
            for (int i = x; i < xend; i++)
            {
                surface_setpixel(surface, i, j, color);
            }
        }
    }
    else
    {
        printf("draw line : (%d , %d) to (%d , %d) [%d #0x%x]\n",
               x, y, xend, yend, theckness, color.raw);
        assert(0 && "todo draw line");
    }
}

void surface_draw_rect(surface_t *surface, rect_t rect, int theckness, color_t color)
{
    surface_draw_line(surface, rect.x, rect.y, rect.x + rect.w - theckness, rect.y, theckness, color);
    surface_draw_line(surface, rect.x, rect.y + rect.h - theckness, rect.x + rect.w - theckness, rect.y + rect.h - theckness, theckness, color);

    surface_draw_line(surface, rect.x, rect.y, rect.x, rect.y + rect.h, theckness, color);
    surface_draw_line(surface, rect.x + rect.w - theckness, rect.y, rect.x + rect.w - theckness, rect.y + rect.h, theckness, color);
}

void surface_copy_rect_alpha(surface_t *dest, int destx, int desty, surface_t *src, rect_t *r)
{
    int start_x = destx;
    int start_y = desty;
    int end_x = min(dest->w, destx + r->w);
    int end_y = min(dest->h, desty + r->h);

    int start_i = r->x;
    int start_j = r->y;

    int i = start_x, j = start_j;
    for (int y = start_y; y < end_y; y++)
    {
        i = start_i;
        for (int x = start_x; x < end_x; x++)
        {
            dest->buffer[y * dest->w + x] = blend_pixel(src->buffer[j * src->w + i++], dest->buffer[y * dest->w + x]).raw;
        }
        j++;
    }
}
