#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gui/container.h>

static void container_paint(widget_t *widget)
{
    (void)widget;
}

container_t *new_container(widget_t *parent, int layout)
{
    container_t *c = calloc(1, sizeof(container_t));
    c->background_color = color_u32(0xffffffff);
    c->widget.layout = layout;
    c->widget.childs = new_list();
    c->widget.paint = container_paint;
    widget_add_child(parent, &c->widget);
    return c;
}
