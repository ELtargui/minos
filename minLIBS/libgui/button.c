#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <gui/button.h>
#include <gui/window.h>
#include <min/mouse.h>

#define HOVERED 2
#define CLECKED 4
#define SELECTED 8
#define DISABLED 16

void button_paint(widget_t *widget)
{
    button_t *button = (button_t *)widget;

    color_t bg = button->background_color;
    if (button->state & CLECKED)
    {
        bg.rgba.r -= 20;
        bg.rgba.g -= 20;
        bg.rgba.b -= 20;
    }
    surface_fill_rect(widget->surface, widget->r, bg);
    if (button->state & SELECTED)
        surface_draw_rect(widget->surface, widget->r, 1, rgba_color(255, 0, 0, 255));

    rect_t r = rect_chrenk(widget->r, 1, 1, 1, 1);
    surface_draw_rect(widget->surface, r, 1, button->border_color);

    if (button->state & HOVERED)
    {
        r = rect_chrenk(r, 1, 1, 1, 1);
        // color_t color = button->background_color;
        // color.rgba.r += 10;
        // color.rgba.g += 10;
        // color.rgba.b += 30;
        surface_draw_rect(widget->surface, r, 1, rgba_color(0, 255, 0, 255));
    }

    ttf_draw_text(button->ttf, button->text, widget->r.x + 1 + 4 + 3, widget->r.y + 4 + 1 + 12 + 2, button->color.raw);
}

void btn_mouse_down(widget_t *widget, int mbtn, int x, int y)
{
    if (mbtn & MOUSE_RBTN)
        return;
    button_t *button = (button_t *)widget;
    button->state |= CLECKED;
    // printf("down\n");
    widget_update(widget);
}

void btn_mouse_up(widget_t *widget, int mbtn, int x, int y)
{
    if (mbtn & MOUSE_RBTN)
        return;
    button_t *button = (button_t *)widget;
    if (button->state & CLECKED)
    {
        button->state &= ~CLECKED;
        // printf("up\n");
        widget_update(widget);
        if (button->on_click)
            button->on_click(button);
    }
}

void btn_enter(widget_t *widget)
{
    button_t *button = (button_t *)widget;
    button->state |= HOVERED;
    // printf("enter\n");
    widget_update(widget);
}

void btn_leave(widget_t *widget)
{
    button_t *button = (button_t *)widget;
    button->state &= ~(HOVERED | CLECKED);
    // printf("leave\n");
    widget_update(widget);
}

button_t *new_button(widget_t *parent, const char *text, void *user)
{
    button_t *button = calloc(1, sizeof(button_t));
    button->text = strdup(text);
    button->color = rgba_color(250, 250, 250, 255);
    button->background_color = rgba_color(88, 88, 88, 255);
    button->border_color = rgba_color(22, 22, 88, 255);
    button->user = user;
    button->widget.r.w = (strlen(text) + 2) * 20;
    button->widget.r.h = 30;
    widget_add_child(parent, &button->widget);
    if (parent)
    {
        button->ttf = window_get_font(parent->win, "Sans");
        button->ttf->surface = button->widget.surface;
        ttf_set_size(button->ttf, 12);
    }
    button->widget.paint = button_paint;
    button->widget.on_mouse_down = btn_mouse_down;
    button->widget.on_mouse_up = btn_mouse_up;
    button->widget.on_enter = btn_enter;
    button->widget.on_leave = btn_leave;

    button->state = 0;
    return button;
}

void button_on_click(button_t *button, void (*on_click)(button_t *), int btn)
{
    button->on_click = on_click;
}
