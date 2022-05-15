#pragma once

#include <gui/widget.h>

typedef struct container
{
    widget_t widget;
    int have_border;
    color_t border_color;
    color_t background_color;
} container_t;

container_t *new_container(widget_t *parent, int layout);
