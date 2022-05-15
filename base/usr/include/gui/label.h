#pragma once

#include <gui/widget.h>
#include <min/truetype.h>

typedef struct label
{
    widget_t widget;
    char *text;
    char *font;
    ttf_t *ttf;
    int size;
    color_t color;
    color_t background_color;
    void *user;
} label_t;

// void label_paint(widget_t *widget);
label_t *new_label(widget_t *parent, const char *text, void *user);
void label_set_font(label_t *label, const char *font);
void label_set_text(label_t *label, const char *text, int size);
void label_set_color(label_t *label, color_t color);
void label_set_background_color(label_t *label, color_t color);
void label_set_fontsize(label_t *label, int size);
void label_on_click(label_t *label, void (*handler)(widget_t *, int, int, int));
