#pragma once

#include <stdint.h>
#include <min/list.h>

typedef struct point
{
    int x;
    int y;
} point_t;

typedef struct Size
{
    int w;
    int h;
} Size_t;

typedef struct rect
{
    int x;
    int y;

    int w;
    int h;
} rect_t;

typedef union color
{
    struct rgba
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    } rgba;
    uint32_t raw;
} color_t;

typedef struct surface
{
    uint32_t *buffer;
    int w;
    int h;
} surface_t;

typedef struct gfx
{
    // struct surface
    // {
    uint32_t *buffer;
    int w;
    int h;
    // };

    uint32_t *backbuffer;
    list_t *clips;
    volatile int spinlock;
} gfx_t;

color_t rgba_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
color_t color_u32(uint32_t raw);
color_t blend_pixel(uint32_t source, uint32_t dest);
color_t blend_pixel_color(color_t source, color_t dest);

gfx_t *gfx_ctx(int w, int h, void *buffer);
void gfx_set_back_buffer(gfx_t *g, void *buf);
void gfx_free(gfx_t *g);
surface_t *gfx_surface(gfx_t *gfx);
void gfx_fill(gfx_t *g, color_t c);
void gfx_fill_rect(gfx_t *g, rect_t rect, color_t color);
void surface_fill_rect(surface_t *s, rect_t rect, color_t color);
void gfx_blit(gfx_t *g);
void gfx_draw_surface(gfx_t *g, int destx, int desty, surface_t *surface);
void gfx_draw_surface_alpha(gfx_t *g, int destx, int desty, surface_t *surface);
void gfx_draw_surface_rect(gfx_t *g, int destx, int desty, surface_t *surface, rect_t *r);
void gfx_draw_surface_rect_alpha(gfx_t *g, int destx, int desty, surface_t *surface, rect_t *r);
void gfx_add_clip(gfx_t *g, rect_t *clip);
void gfx_blit_clips(gfx_t *g);
void gfx_blit_rect(gfx_t *g, rect_t *r);

rect_t Rectangle(int x, int y, int w, int h);
rect_t *new_rect(int x, int y, int w, int h);
rect_t rect_get_intersection(rect_t r1, rect_t r2);
rect_t rect_chrenk(rect_t r, int top, int bottom, int left, int right);
int point_in_rect(int x, int y, rect_t r);

gfx_t *surface_gfx(surface_t *surface);

surface_t *surface_from_buffer(void *buffer, int w, int h);
surface_t *new_surface(int w, int h);
void surface_fill(surface_t *s, color_t color);
void surface_copy_rect_alpha(surface_t *dest, int destx, int desty, surface_t *src, rect_t *r);
void surface_draw_line(surface_t *surface, int x, int y, int xend, int yend, int theckness, color_t color);
void surface_draw_rect(surface_t *surface, rect_t rect, int theckness, color_t color);

surface_t *load_png(const char *file);
surface_t *load_jpeg(const char *filename);

void surface_setpixel(surface_t *s, int x, int y, color_t c);
color_t surface_getpixel(surface_t *s, int x, int y);
