#pragma once
#include <gui/widget.h>
#include <min/truetype.h>

typedef struct button
{
    widget_t widget;
    char *text;
    ttf_t *ttf;
    color_t color;
    color_t background_color;
    color_t border_color;
    int state;
    void *user;

    void (*on_click)(struct button *);
} button_t;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gui/button.h>

void button_paint(widget_t *widget);

button_t *new_button(widget_t *parent, const char *text, void *user);
void button_on_click(button_t *button, void (*on_click)(button_t *), int btn);
