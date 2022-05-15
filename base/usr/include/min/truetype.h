#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <min/gfx.h>

typedef struct ttf
{
    uint8_t *buffer;
    uint8_t *bend;
    uint8_t *p;
    size_t size;

    uint8_t *head;
    uint8_t *cmap;
    uint8_t *glyf;
    uint8_t *loca;
    uint8_t *hhea;
    uint8_t *hmtx;
    uint8_t *maxp;
    uint8_t *name;

    uint32_t cmap_len;
    uint32_t glyf_len;
    uint32_t loca_len;

    uint32_t camp_subtable;
    float unitsPerEm;
    int16_t indexToLocFormat;
    float scale;
    uint16_t numOfLongHorMetrics;
    int num_glyphs;
    surface_t *surface;
    char *fontname;
} ttf_t;

surface_t *ttf_init_ctx(ttf_t *ttf, void *buffer, int w, int h);
void ttf_set_size(ttf_t *ttf, float size);
int ttf_text_width(ttf_t *ttf, const char *s);
int ttf_draw_text(ttf_t *ttf, const char *s, int x, int y, uint32_t color);
int ttf_draw_glyph(ttf_t *ttf, uint32_t cp, int x, int y, uint32_t color);
int ttf_draw_glyph_index(ttf_t *ttf, uint32_t index, int x, int y, uint32_t color);
ttf_t *ttf_load(const char *font);
