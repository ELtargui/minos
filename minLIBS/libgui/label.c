#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gui/window.h>
#include <gui/label.h>

static void label_paint(widget_t *widget)
{
    label_t *label = (label_t *)widget;

    // int w = ttf_text_width(label->ttf, label->text) + 2;
    // int h = label->size + 4;
    // widget->r.w = w;
    // widget->r.h = h;
    {
        surface_fill_rect(widget->surface, widget->r, label->background_color);
    }

    if (label->text)
        ttf_draw_text(label->ttf, label->text, widget->r.x + 1, widget->r.y + label->size, label->color.raw);
}

label_t *new_label(widget_t *parent, const char *text, void *user)
{
    label_t *label = calloc(1, sizeof(label_t));
    label->color = color_u32(0xff000000);
    if (parent)
    {
        label->background_color = ((window_t *)parent->win)->color;
    }
    label->widget.paint = label_paint;
    label->widget.r.w = 50;
    label->widget.r.h = 30;
    label->user = user;
    widget_add_child(parent, &label->widget);
    label_set_text(label, text, 12);
    return label;
}

void label_set_font(label_t *label, const char *font)
{
    if (label->widget.win)
    {
        label->ttf = window_get_font(label->widget.win, "Sans");
        label->ttf->surface = label->widget.surface;
    }
}

void label_set_text(label_t *label, const char *text, int size)
{
    if (label->text)
    {
        free(label->text);
        label->text = NULL;
    }

    if (!label->font)
    {
        label_set_font(label, "Sans");
    }

    ttf_set_size(label->ttf, size);
    label->size = size;
    if (text)
    {
        label->text = strdup(text);
        // int len = 5 + (size / 2) * strlen(label->text);
        // if (label->widget.r.w < len)
        // {
        //     label->widget.r.w = len;
        // }
    }
    // label->widget.r.h = size;
}

void label_set_color(label_t *label, color_t color)
{
    label->color = color;
}

void label_set_background_color(label_t *label, color_t color)
{
    label->background_color = color;
}

void label_set_fontsize(label_t *label, int size)
{
    label->size = size;
    if (label->text)
    {
        label->widget.r.w = 5 + (size / 2) * strlen(label->text);
        label->widget.r.h = size;
    }

    if (label->ttf)
        ttf_set_size(label->ttf, size);
}

void label_on_click(label_t *label, void (*handler)(widget_t *, int, int, int))
{
    widget_t *widget = &label->widget;
    widget->on_mouse_down = handler;
}
