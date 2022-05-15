#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <gui/app.h>
#include <gui/window.h>
#include <gui/winmsg.h>
#include <gui/container.h>
#include <min/truetype.h>

extern void app_add_window(app_t *app, window_t *win);

window_t *new_window(app_t *app, int w, int h, int frameless)
{
    window_t *win = calloc(1, sizeof(window_t));
    win->app = app;
    win->win_id = app->window_list->size + 1;

    app_add_window(app, win);

    win->rect = Rectangle(100, 100, w, h);
    msg_new_window_t msg;
    msg.rect = win->rect;
    msg.frameless = frameless;
    msg.winid = win->win_id;
    app_send_msg(win->app, NEW_WINDOW, &msg, sizeof(msg_new_window_t));

    {
        char buf[512];
        int e = app_recv(app, buf);
        assert(e == WINDOW_SET_INFO);
        ipc_msg_t *mp = (ipc_msg_t *)buf;
        msg_window_info_t *msg = (msg_window_info_t *)mp->data;
        int fd = shm_open(msg->shared_buffer, O_RDWR, 0);
        if (fd == -1)
        {
            perror("open shm");
            assert(0);
        }
        void *buffer = mmap(NULL, msg->rect.w * msg->rect.h * sizeof(color_t), PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
        if (buffer == MAP_FAILED)
        {
            perror("mmap shm obj ");
            assert(0);
        }
        win->surface = surface_from_buffer(buffer, msg->rect.w, msg->rect.h);
    }

    win->color = color_u32(0xffffffff);
    win->fonts = new_list();

    container_t *container = new_container(NULL, 0);
    window_set_root_widget(win, &container->widget);

    return win;
}

ttf_t *window_get_font(window_t *win, const char *font)
{
    assert(font);

    foreach (win->fonts, n)
    {
        ttf_t *ttf = n->value;
        if (!strcmp(ttf->fontname, font))
        {
            return ttf;
        }
    }

    char path[256];
    sprintf(path, "/usr/share/fonts/DejaVu%s.ttf", font);
    ttf_t *ttf = ttf_load(path);
    if (!ttf)
    {
        perror("load font");
        return NULL;
    }

    ttf->fontname = strdup(font);

    list_append(win->fonts, ttf);
    return ttf;
}

window_t *window_from_id(app_t *app, uint32_t id)
{
    foreach (app->window_list, n)
    {
        window_t *w = n->value;
        if (w->win_id == id)
        {
            return w;
        }
    }

    return NULL;
}

void window_set_position(window_t *win, int x, int y)
{
    win->rect.x = x;
    win->rect.y = y;
}

void window_set_size(window_t *win, int w, int h)
{
    win->rect.w = w;
    win->rect.h = h;
}

void window_set_backgound_color(window_t *win, color_t color)
{
    win->color = color;
    surface_fill(win->surface, win->color);
}

void window_show(window_t *win)
{
    widget_paint(win->root);

    msg_show_window_t msg;
    msg.rect = win->rect;
    msg.frameless = win->frameless;
    msg.winid = win->win_id;
    app_send_msg(win->app, WINDOW_SHOW, &msg, sizeof(msg_show_window_t));
}

void window_flush_rect(window_t *win, rect_t *r)
{
    msg_window_flush_rect_t msg;
    msg.winid = win->win_id;
    msg.rect = *r;
    app_send_msg(win->app, WINDOW_RECT, &msg, sizeof(msg_window_flush_rect_t));
}

void window_set_info()
{
    assert(0 && "todo");
}

void win_handle_mouse(window_t *win, ipc_event_t event, msg_mouse_event_t *msg)
{
    widget_t *w = widget_at_position(win->root, msg->x, msg->y);
    if (w != win->hovered)
    {
        if (win->hovered)
        {
            //leave
            widget_leave(win->hovered);
        }
        win->hovered = w;
        if (win->hovered)
        {
            //enter
            widget_enter(win->hovered);
        }
    }

    if (event == MOUSE_MOVE)
    {
        widget_mouse_move(w, msg->x, msg->y);
    }
    else if (event == MOUSE_CLICK)
    {
        widget_mouse_down(w, msg->btn, msg->x, msg->y);
    }
    else if (event == MOUSE_RELEASE)
    {
        widget_mouse_up(w, msg->btn, msg->x, msg->y);
    }
}

void win_handle_msg(app_t *app, ipc_msg_t *msg)
{
    ipc_event_t event = msg->event;

    msg_head_t *head = (msg_head_t *)msg->data;
    window_t *win = window_from_id(app, head->winid);
    if (!win)
    {
        assert(0 && "inavalide window id");
    }

    switch (event)
    {
    case MOUSE_MOVE:
    case MOUSE_CLICK:
    case MOUSE_RELEASE:
        win_handle_mouse(win, event, (msg_mouse_event_t *)msg->data);
        break;

    case WIN_ENTER:
        // printf("win enter:%p\n", win);
        break;
    case WIN_LEAVE:
        // printf("win leave:%p\n", win);
        break;
    case WINDOW_SET_INFO:
        window_set_info(win, (msg_window_info_t *)msg->data);
        break;
    default:
        printf("gui[%x:%d]:unhandled event %d\n", app->app_id, win->win_id, event);
        break;
    }
}

void window_set_root_widget(window_t *win, widget_t *widget)
{
    widget->r = Rectangle(0, 0, win->rect.w, win->rect.h);
    widget->surface = win->surface;
    win->root = widget;
    widget->win = win;
}
