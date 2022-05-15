#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <gui/widget.h>
#include <gui/window.h>

void widget_add_child(widget_t *widget, widget_t *child)
{
    assert(child);
    child->parent = widget;
    if (widget)
    {
        assert(widget->childs);
        assert(widget->surface);

        child->surface = widget->surface;
        child->win = widget->win;
        child->parent = widget;
        list_append(widget->childs, child);

        if (widget->layout)
            widget_apply_layout(widget);
        else
            child->r = widget->r;
    }
}

void widget_paint(widget_t *widget)
{
    assert(widget);
    widget->paint(widget);
    if (widget->childs)
    {
        widget_apply_layout(widget);
        foreach (widget->childs, n)
        {
            widget_t *w = n->value;
            widget_paint(w);
        }
    }
}

widget_t *widget_at_position(widget_t *root, int x, int y)
{
    if (root->childs)
    {
        foreach (root->childs, n)
        {
            widget_t *w = n->value;
            if (!point_in_rect(x, y, w->r))
            {
                return widget_at_position(w, x, y);
            }
        }
    }
    return root;
}

void widget_enter(widget_t *widget)
{
    // printf("enter:%p\n", widget);
    if (widget->on_enter)
    {
        widget->on_enter(widget);
    }
}

void widget_leave(widget_t *widget)
{
    // printf("leave:%p\n", widget);
    if (widget->on_leave)
    {
        widget->on_leave(widget);
    }
}

void widget_mouse_down(widget_t *widget, int btn, int x, int y)
{
    if (widget->on_mouse_down)
        widget->on_mouse_down(widget, btn, x, y);
}

void widget_mouse_up(widget_t *widget, int btn, int x, int y)
{
    if (widget->on_mouse_up)
        widget->on_mouse_up(widget, btn, x, y);
}

void widget_mouse_move(widget_t *widget, int x, int y)
{
    if (widget->on_mouse_move)
        widget->on_mouse_move(widget, x, y);
}

void horizontal_layout(widget_t *widget)
{
    int x = widget->r.x + 1;
    int endline = x + widget->r.w;
    // printf("apply H.line layout [%d:%d]\n", x, endline);

    foreach (widget->childs, n)
    {
        widget_t *w = n->value;
        w->r.x = x;
        w->r.y = widget->r.y + 2;
        w->r.h = widget->r.h - 2;
        // printf("  [%d,%d %dx%d]\n", w->r.x, w->r.y, w->r.w, w->r.h);
        x += w->r.w + 1;
        if (x > endline)
        {
            break;
        }
    }
}

void virtical_layout(widget_t *widget)
{
    int y = widget->r.y + 2;
    int endline = widget->r.y + widget->r.h;
    // printf("apply V.line layout [%d:%d]\n", y, endline);

    foreach (widget->childs, n)
    {
        widget_t *w = n->value;
        w->r.y = y;
        w->r.x = widget->r.x + 1;
        w->r.w = widget->r.w - 2;
        // printf("  [%d,%d %dx%d]\n", w->r.x, w->r.y, w->r.w, w->r.h);

        y += w->r.h + 2;
        if (y > endline)
        {
            break;
        }
    }
}

void widget_apply_layout(widget_t *widget)
{
    switch (widget->layout)
    {
    case 1:
        horizontal_layout(widget);
        break;
    case 2:
        virtical_layout(widget);
        break;
    default:
        break;
    }
}

void widget_set_layout(widget_t *widget, int layout, int flags)
{
    widget->layout = layout;
    widget->layout_flags = flags;
    if (widget->childs)
    {
        widget_apply_layout(widget);
    }
}

void widget_set_size(widget_t *widget, int w, int h)
{
    widget->r.w = w;
    widget->r.h = h;
    if (widget->parent && widget->parent->layout)
    {
        widget_apply_layout(widget->parent);
    }
}

void widget_update(widget_t *widget)
{
    widget_paint(widget);
    window_flush_rect((window_t *)widget->win, &widget->r);
}
