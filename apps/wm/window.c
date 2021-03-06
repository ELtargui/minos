#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <min/mouse.h>
#include <min/ipc.h>
#include <min/list.h>
#include <sys/mman.h>
#include "window.h"

list_t *app_list = NULL;
static s_window_t *sys_menu = NULL;
static int sys_menu_visible = 0;
app_t *app_from_id(uint32_t app_id)
{
    foreach (app_list, n)
    {
        app_t *app = n->value;
        if (app->app_id == app_id)
            return app;
    }
    return NULL;
}

s_window_t *win_from_id(app_t *app, uint32_t id)
{
    foreach (app->window_list, n)
    {
        s_window_t *w = n->value;
        if (w->win_id == id)
            return w;
    }
    return NULL;
}

void handle_app_connect(uint32_t app_id)
{
    if (app_from_id(app_id))
    {
        assert(0 && "exist");
    }
    app_t *app = calloc(1, sizeof(app_t));
    app->app_id = app_id;
    app->window_list = new_list();
    list_append(app_list, app);
    if (app_send_msg_to(server_app, app->app_id, APP_HELLO, NULL, 0))
    {
        assert(0 && "send msg");
    }
}

void handle_new_window(app_t *app, msg_new_window_t *msg)
{
    s_window_t *win = calloc(1, sizeof(s_window_t));

    win->x = msg->rect.x;
    win->y = msg->rect.y;
    win->app = app;
    win->win_id = msg->winid;
    win->frameless = msg->frameless;

    win->rects = new_list();
    pthread_mutex_init(&win->mutex, NULL);
    char name[64];
    sprintf(name, "%x_%d_%dx%d", app->app_id, win->win_id, msg->rect.w, msg->rect.h);
    printf("shared buffer name %s\n", name);
    int fd = shm_open(name, O_RDWR | O_CREAT, 0666);
    if (fd == -1)
    {
        printf("wm error\n");
        perror("open shm obj");
        assert(0);
    }
    size_t size = msg->rect.w * msg->rect.h * sizeof(color_t);
    if (ftruncate(fd, size) == -1)
    {
        perror("ftruncate shm");
        assert(0);
    }

    void *buffer = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED)
    {
        perror("mmap shm obj");
        assert(0);
    }
    win->surface = surface_from_buffer(buffer, msg->rect.w, msg->rect.h);

    msg_window_info_t rmsg;
    rmsg.winid = win->win_id;
    rmsg.rect = (rect_t){win->x, win->y, win->surface->w, win->surface->h};
    strcpy(rmsg.shared_buffer, name);
    if (app_send_msg_to(server_app, app->app_id, WINDOW_SET_INFO, &rmsg, sizeof(msg_window_info_t)))
    {
        assert(0);
    }

    list_append(app->window_list, win);
    printf("win [%p:%d] created\n", win->app, win->win_id);
}

void handle_window_flush_rect(app_t *app, msg_window_flush_rect_t *msg)
{
    // printf("flush rect app:%p win:%d \n", app, msg->winid);
    s_window_t *win = win_from_id(app, msg->winid);
    assert(win);
    win_redraw_rect(win, &msg->rect);
}

void handle_window_show(app_t *app, s_window_t *win, msg_show_window_t *msg)
{
    (void)app;
    win->x = msg->rect.x;
    win->y = msg->rect.y;
    win_show(win);
}

void handle_window_hide(s_window_t *win)
{
    win_hide(win);
}
void init_win()
{
    app_list = new_list();
}

rect_t win_rect(s_window_t *win)
{
    return Rectangle(win->x, win->y, win->surface->w, win->surface->h);
}

rect_t win_self_rect(s_window_t *win)
{
    return Rectangle(0, 0, win->surface->w, win->surface->h);
}

void win_redraw_rect(s_window_t *win, rect_t *r)
{
    pthread_mutex_lock(&win->mutex);
    rect_t *rect = new_rect(r->x, r->y, r->w, r->h);
    list_append(win->rects, rect);
    pthread_mutex_unlock(&win->mutex);
}
void win_redraw(s_window_t *win)
{
    pthread_mutex_lock(&win->mutex);
    rect_t *rect = new_rect(0, 0, win->surface->w, win->surface->h);
    while (win->rects->head)
    {
        list_node_t *n = list_take_first_node(win->rects);

        free(n->value);
        free(n);
    }

    list_append(win->rects, rect);
    pthread_mutex_unlock(&win->mutex);
}

void win_show(s_window_t *win)
{
    win_redraw(win);
    stack_add_window(win);
    win_send_value_int(win, WINDOW_SET_VISIBLE, 1);
}

void win_send_value_int(s_window_t *win, ipc_event_t event, int value)
{
    msg_set_value_t msg;
    msg.winid = win->win_id;
    msg.value_int = value;
    app_send_msg_to(server_app, win->app->app_id, WIN_ENTER, &msg, sizeof(msg_head_t));
}

void win_hide(s_window_t *win)
{
    pthread_mutex_lock(&win->mutex);
    stack_remove_window(win);
    pthread_mutex_unlock(&win->mutex);
    win_send_value_int(win, WINDOW_SET_VISIBLE, 0);
    screen_redraw_rect(win_rect(win));
}

void win_enter(s_window_t *win)
{
    msg_head_t msg;
    msg.winid = win->win_id;
    if (app_send_msg_to(server_app, win->app->app_id, WIN_ENTER, &msg, sizeof(msg_head_t)))
    {
        assert(0);
    }
}

void win_leave(s_window_t *win)
{
    msg_head_t msg;
    msg.winid = win->win_id;
    if (app_send_msg_to(server_app, win->app->app_id, WIN_LEAVE, &msg, sizeof(msg_head_t)))
    {
        assert(0);
    }
}

void win_start_move(s_window_t *win, msg_win_move_t *msg)
{
    set_moving_window(win);
}

void win_end_move(s_window_t *win)
{
    if (win != get_moving_window())
    {
        printf("error :end window move [%p moving:%p]\n", win, get_moving_window());
    }
    assert(win == get_moving_window());
    set_moving_window(NULL);
}

void win_move_to(s_window_t *win, int x, int y)
{
    rect_t r = win_rect(win);
    win->x = x;
    win->y = y;
    screen_redraw_rect(r);
    win_redraw(win);
}

void win_set_system_menu(s_window_t *win)
{
    if (sys_menu)
    {
        msg_sys_menu_t msg;
        msg.accepted = 0;
        msg.error = 1;
        if (app_send_msg_to(server_app, win->app->app_id, SYS_MENU, &msg, sizeof(msg_sys_menu_t)))
        {
            assert(0);
        }
    }
    sys_menu = win;
    msg_sys_menu_t msg;
    msg.accepted = 1;
    msg.error = 0;
    if (app_send_msg_to(server_app, win->app->app_id, SYS_MENU, &msg, sizeof(msg_sys_menu_t)))
    {
        assert(0);
    }
}

