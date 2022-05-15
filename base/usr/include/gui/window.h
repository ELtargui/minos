#pragma once

#include <min/gfx.h>
#include <min/truetype.h>
#include <gui/app.h>
#include <gui/widget.h>

typedef struct window
{
    app_t *app;
    uint32_t win_id;
    rect_t rect;
    color_t color;
    int frameless;

    surface_t *surface;
    widget_t *root;
    widget_t *hovered;
    int m_btn;

    list_t *fonts;
} window_t;

window_t *new_window(app_t *app, int w, int h, int frameless);
window_t *window_from_id(app_t *app, uint32_t id);
void window_set_position(window_t *win, int x, int y);
void window_set_size(window_t *win, int w, int h);
void window_set_backgound_color(window_t *win, color_t color);
void window_show(window_t *win);
void window_set_root_widget(window_t *win, widget_t *widget);
void window_flush_rect(window_t *win, rect_t *r);
ttf_t *window_get_font(window_t *win, const char *font);
