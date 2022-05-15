#pragma once

#include <min/gfx.h>
#include <gui/app.h>
#include <gui/winmsg.h>
#include <pthread.h>

typedef struct s_window
{
    app_t *app;
    uint32_t win_id;
    int x;
    int y;
    int frameless;
    surface_t *surface;
    list_t *rects;
    pthread_mutex_t mutex;
} s_window_t;

extern app_t *server_app;

void stack_add_window(s_window_t *win);
void stack_remove_window(s_window_t *win);
void set_mouse_pos(int x, int y);

app_t *app_from_id(uint32_t app_id);
void init_win();

s_window_t *win_from_id(app_t *app, uint32_t id);
void win_show(s_window_t *win);
void win_hide(s_window_t *win);
void win_redraw_rect(s_window_t *win, rect_t *r);
rect_t win_rect(s_window_t *win);
rect_t win_self_rect(s_window_t *win);

void handle_app_connect(uint32_t app_id);
void handle_new_window(app_t *app, msg_show_window_t *msg);
void handle_window_flush_rect(app_t *app, msg_window_flush_rect_t *msg);
void handle_window_show(app_t *app, s_window_t *win, msg_show_window_t *msg);

void win_enter(s_window_t *win);
void win_leave(s_window_t *win);
s_window_t *top_window_in_pos(int x, int y);
