#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <gui/window.h>
#include <gui/widget.h>
#include <min/mouse.h>

void default_exit(frame_t *frame)
{
    window_t *win = frame->widget.win;
    printf("request win exit (%p) from frame\n", win);
    // window_exit(win);
}

void default_click(frame_t *frame, int x, int y)
{
    window_t *win = frame->widget.win;
    // window_show_menu(win, menu);
    printf("%p show menu\n", win);
}

void frame_paint(widget_t *widget)
{
    frame_t *frame = (frame_t *)widget;
    window_t *win = widget->win;
    surface_fill_rect(win->surface, Rectangle(0, 0, frame->widget.r.w, frame->height), rgba_color(255, 255, 0, 255));
    surface_draw_rect(win->surface, frame->widget.r, 1, rgba_color(255, 0, 255, 255));
}

void frame_mouse_down(widget_t *widget, int mbtn, int x, int y)
{
    frame_t *frame = (frame_t *)widget;
    if (mbtn & MOUSE_RBTN)
    {
        frame->on_click(frame, x, y);
    }
    else if (mbtn & MOUSE_LBTN)
    {
        window_t *win = widget->win;
        window_start_move(win);
        // printf("win %p start move\n", win->win_id);
        frame->moving = 1;
    }
}

void frame_mouse_up(widget_t *widget, int mbtn, int x, int y)
{
    frame_t *frame = (frame_t *)widget;
    if (mbtn & MOUSE_LBTN)
    {
        window_t *win = widget->win;
        window_end_move(win);
        // printf("win %p end move\n", win->win_id);
        frame->moving = 0;
    }
}

void frame_enter(widget_t *widget)
{
    printf("enter\n");
}

void frame_leave(widget_t *widget)
{
    frame_t *frame = (frame_t *)widget;
    printf("leave\n");
    if (frame->moving)
    {
        frame->moving = 0;
        window_end_move(widget->win);
        // printf("win %p end move\n", widget->win);
    }
}

frame_t *init_window_frame(window_t *win)
{
    frame_t *frame = calloc(1, sizeof(frame_t));
    frame->on_exit = default_exit;
    frame->on_click = default_click;

    frame->widget.paint = frame_paint;
    frame->widget.on_mouse_down = frame_mouse_down;
    frame->widget.on_mouse_up = frame_mouse_up;
    frame->widget.on_enter = frame_enter;
    frame->widget.on_leave = frame_leave;
    frame->widget.win = win;
    frame->height = 20;

    frame->widget.r = Rectangle(0, 0, win->rect.w, win->rect.h);
    return frame;
}
