#include <stdint.h>
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

gfx_t *gfx_ctx(int w, int h, void *buffer)
{
    gfx_t *g = malloc(sizeof(gfx_t));
    g->backbuffer = buffer;
    g->buffer = buffer;
    g->w = w;
    g->h = h;

    g->clips = new_list();
    return g;
}

void gfx_set_back_buffer(gfx_t *g, void *buf)
{
    g->buffer = buf;
}

void gfx_free(gfx_t *g)
{
    if (g->backbuffer != g->buffer)
        free(g->backbuffer);
    list_delete_all(g->clips, free);
    free(g->clips);
    free(g);
}

surface_t *gfx_surface(gfx_t *gfx)
{
    return (surface_t *)gfx;
}

gfx_t *surface_gfx(surface_t *surface)
{
    return gfx_ctx(surface->w, surface->h, surface->buffer);
}

void gfx_setpixel(gfx_t *g, int x, int y, color_t c)
{
    if (x > g->w || y > g->h)
        assert(0);
    g->buffer[y * g->w + x] = c.raw;
}

color_t gfx_getpixel(gfx_t *g, int x, int y)
{
    if (x > g->w || y > g->h)
        assert(0);
    return color_u32(g->buffer[y * g->w + x]);
}

void gfx_fill(gfx_t *g, color_t c)
{
    for (int y = 0; y < g->h; y++)
    {
        for (int x = 0; x < g->w; x++)
        {
            g->buffer[y * g->w + x] = c.raw;
        }
    }
}

void gfx_fill_rect(gfx_t *g, rect_t rect, color_t color)
{
    for (int y = rect.y; y < rect.y + rect.h; y++)
    {
        for (int x = rect.x; x < rect.x + rect.w; x++)
        {
            g->buffer[y * g->w + x] = color.raw;
        }
    }
}

void gfx_blit(gfx_t *g)
{
    if (g->buffer == g->backbuffer)
        return;
    memcpy(g->backbuffer, g->buffer, g->w * g->h * sizeof(color_t));
}

void gfx_blit_rect(gfx_t *g, rect_t *r)
{
    if (g->buffer == g->backbuffer)
        return;
    int my = min(r->y + r->h, g->h);
    int mx = min(r->x + r->w, g->w);

    for (int y = r->y; y < my; y++)
    {
        for (int x = r->x; x < mx; x++)
        {
            g->backbuffer[y * g->w + x] = g->buffer[y * g->w + x];
        }
    }
}

void gfx_add_clip(gfx_t *g, rect_t *clip)
{
    int x = max(0, clip->x);
    int y = max(0, clip->y);
    int w = min(g->w, x + clip->w) - x;
    int h = min(g->h, y + clip->h) - y;

    if (w <= 0 || h <= 0)
        return;

    list_append(g->clips, new_rect(x, y, w, h));
}

void gfx_blit_clips(gfx_t *g)
{
    // TODO: optimisme Rectangless

    while (g->clips->head)
    {
        rect_t *r = list_take_last(g->clips);
        gfx_blit_rect(g, r);
        free(r);
    }
}

void gfx_draw_surface(gfx_t *g, int destx, int desty, surface_t *surface)
{
    rect_t r = Rectangle(0, 0, surface->w, surface->h);
    gfx_draw_surface_rect(g, destx, desty, surface, &r);
}

void gfx_draw_surface_alpha(gfx_t *g, int destx, int desty, surface_t *surface)
{
    rect_t r = Rectangle(0, 0, surface->w, surface->h);
    gfx_draw_surface_rect_alpha(g, destx, desty, surface, &r);
}

void gfx_draw_surface_rect(gfx_t *g, int destx, int desty, surface_t *surface, rect_t *r)
{
    int start_x = destx;
    int start_y = desty;
    int end_x = min(g->w, destx + r->w);
    int end_y = min(g->h, desty + r->h);

    int start_i = r->x;
    int start_j = r->y;

    int i = start_x, j = start_j;
    for (int y = start_y; y < end_y; y++)
    {
        i = start_i;
        for (int x = start_x; x < end_x; x++)
        {
            g->buffer[y * g->w + x] = surface->buffer[j * surface->w + i++];
        }
        j++;
    }
}

void gfx_draw_surface_rect_alpha(gfx_t *g, int destx, int desty, surface_t *surface, rect_t *r)
{
    int start_x = destx;
    int start_y = desty;
    int end_x = min(g->w, destx + r->w);
    int end_y = min(g->h, desty + r->h);

    int start_i = r->x;
    int end_i = min(r->x + r->w, surface->w);
    int start_j = r->y;
    int end_j = min(r->y + r->h, surface->h);

    end_x = min(start_x + (end_i - start_i), end_x);
    end_y = min(start_y + (end_j - start_j), end_y);
    int i = start_x, j = start_j;
    for (int y = start_y; y < end_y; y++)
    {
        i = start_i;
        for (int x = start_x; x < end_x; x++)
        {
            gfx_setpixel(g, x, y, blend_pixel(surface_getpixel(surface, i++, j).raw, gfx_getpixel(g, x, y).raw));
        }
        j++;
    }
}
