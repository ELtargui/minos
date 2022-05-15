#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <min/gfx.h>

color_t rgba_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    color_t c;
    c.rgba.r = r;
    c.rgba.g = g;
    c.rgba.b = b;
    c.rgba.a = a;
    return c;
}

color_t color_u32(uint32_t raw)
{
    color_t c;
    c.raw = raw;
    return c;
}

color_t blend_pixel(uint32_t source, uint32_t dest)
{
    return blend_pixel_color(color_u32(source), color_u32(dest));
}

color_t blend_pixel_color(color_t source, color_t dest)
{
    if (!dest.rgba.a || source.rgba.a == 255)
        return source;

    if (!source.rgba.a)
        return dest;

    color_t r;
    int alpha = source.rgba.a + 1;
    int inv_alpha = 256 - source.rgba.a;

    r.rgba.r = (unsigned char)((alpha * source.rgba.r + inv_alpha * dest.rgba.r) / 256);
    r.rgba.g = (unsigned char)((alpha * source.rgba.g + inv_alpha * dest.rgba.g) / 256);
    r.rgba.b = (unsigned char)((alpha * source.rgba.b + inv_alpha * dest.rgba.b) / 256);
    r.rgba.a = 255;

    return r;
}
