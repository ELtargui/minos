#pragma once

#include <min/gfx.h>

typedef struct widget
{
    rect_t r;
    int fixed_width;
    int fixed_height;
    
    struct widget *parent;
    list_t *childs;
    uint16_t layout;
    uint16_t layout_flags;

    void (*paint)(struct widget *);
    void (*on_mouse_down)(struct widget *, int, int, int);
    void (*on_mouse_up)(struct widget *, int, int, int);
    void (*on_mouse_move)(struct widget *, int, int);
    void (*on_enter)(struct widget *);
    void (*on_leave)(struct widget *);
    surface_t *surface;
    void *win;
} widget_t;

void widget_add_child(widget_t *widget, widget_t *child);
void widget_paint(widget_t *widget);
widget_t *widget_at_position(widget_t *root, int x, int y);
void widget_enter(widget_t *widget);
void widget_leave(widget_t *widget);
void widget_mouse_down(widget_t *widget, int btn, int x, int y);
void widget_mouse_up(widget_t *widget, int btn, int x, int y);
void widget_mouse_move(widget_t *widget, int x, int y);
void widget_set_layout(widget_t *widget, int layout, int flags);
void widget_set_size(widget_t *widget, int w, int h);
void widget_apply_layout(widget_t *widget);
void widget_update(widget_t *widget);
