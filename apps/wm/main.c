#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <assert.h>
#include <min/mouse.h>
#include <min/ipc.h>
#include <min/list.h>
#include <gui/winmsg.h>
#include "window.h"

extern gfx_t *start_compositor();

int mouse_fd;
int kbd_fd;
int ipc_fd;
int g_running = 1;
app_t *server_app;
gfx_t *gfx = NULL;

static s_window_t *moving_window;
static int move_dx = 0;
static int move_dy = 0;

static int min(int a, int b)
{
    return a < b ? a : b;
}

static int max(int a, int b)
{
    return a > b ? a : b;
}

void set_moving_window(s_window_t *win)
{
    if (win)
    {
        int mx, my;
        get_mouse_pos(&mx, &my);

        move_dx = mx - win->x;
        move_dy = my - win->y;
        printf("%p init moving dx:%d dy:%d [%d,%d]\n", win, move_dx, move_dy, win->x, win->y);
    }
    printf("set moving_window=%p\n", win);
    moving_window = win;
}

s_window_t *get_moving_window()
{
    return moving_window;
}

void send_mouse_event(s_window_t *win, ipc_event_t e, int x, int y, int btn)
{
    msg_mouse_event_t msg;

    msg.winid = win->win_id;
    msg.btn = btn;
    msg.x = x;
    msg.y = y;
    if (app_send_msg_to(server_app, win->app->app_id, e, &msg, sizeof(msg_mouse_event_t)))
    {
        assert(0);
    }
}

static int m_x = 0;
static int m_y = 0;

void handle_mouse()
{
    mouse_packet_t mp;
    int e = read(mouse_fd, &mp, sizeof(mouse_packet_t));
    if (e != sizeof(mouse_packet_t))
    {
        assert(0 && "read mouse");
    }
    assert(mp.magic = MOUSE_MAGIC);

    static int m_btn = 0;
    static s_window_t *win = NULL;
    int is_mouving = 0;
    if (mp.dx || mp.dy)
    {
        m_x = max(0, min(m_x + mp.dx, gfx->w));
        m_y = max(0, min(m_y + mp.dy, gfx->h));
        is_mouving = 1;
        set_mouse_pos(m_x, m_y);
    }
    if (!moving_window)
    {
        s_window_t *tmp = top_window_in_pos(m_x, m_y);
        if (win != tmp)
        {
            if (moving_window && moving_window != win)
                if (win)
                {
                    // leave
                    // printf("leave::%p\n", win);
                    win_leave(win);
                }
            win = tmp;
            if (win)
            {
                // enter
                // printf("enter::%p\n", win);
                win_enter(win);
            }
        }
    }

    int x = 0;
    int y = 0;
    if (win)
    {
        x = m_x - win->x;
        y = m_y - win->y;
    }
    if (m_btn != mp.btn)
    {
        if (win)
        {
            if (!(m_btn & MOUSE_LBTN) && (mp.btn & MOUSE_LBTN))
            {
                // printf("[L down]\n");
                send_mouse_event(win, MOUSE_CLICK, x, y, MOUSE_LBTN);
            }
            if (!(m_btn & MOUSE_RBTN) && (mp.btn & MOUSE_RBTN))
            {
                /*click RBTN*/
                // printf("[R down]\n");
                send_mouse_event(win, MOUSE_CLICK, x, y, MOUSE_RBTN);
            }

            if ((m_btn & MOUSE_LBTN) && !(mp.btn & MOUSE_LBTN))
            {
                // printf("[L up]\n");
                send_mouse_event(win, MOUSE_RELEASE, x, y, MOUSE_LBTN);
            }
            if ((m_btn & MOUSE_RBTN) && !(mp.btn & MOUSE_RBTN))
            {
                /*release RBTN*/
                // printf("[R up]\n");
                send_mouse_event(win, MOUSE_RELEASE, x, y, MOUSE_RBTN);
            }
        }
        m_btn = mp.btn;
    }

    if (moving_window)
    {
        win_move_to(moving_window, m_x - move_dx, m_y - move_dy);
    }
    else if (is_mouving && win)
    {
        //move
        send_mouse_event(win, MOUSE_MOVE, x, y, mp.btn);
    }
}

void handle_kbd()
{
    uint8_t scancode;
    if (read(kbd_fd, &scancode, 1) == -1)
    {
        assert(0 && "read kbd");
    }
    printf("kbd %d\n", scancode);
}

void handle_ipc()
{
    uint8_t buf[512];
    int e = read(ipc_fd, buf, 512);
    if (e == -1)
    {
        perror("read ipc");
        assert(0 && "ipc");
    }
    ipc_msg_t *msg = (ipc_msg_t *)buf;
    app_t *app = app_from_id(msg->id);
    s_window_t *win = NULL;
    if (msg->event != APP_CONNECT && msg->event != NEW_WINDOW)
    {
        msg_head_t *head = (msg_head_t *)msg->data;
        win = win_from_id(app, head->winid);
        assert(win);
    }
    switch (msg->event)
    {
    case APP_CONNECT:
        handle_app_connect(msg->id);
        break;
    case NEW_WINDOW:
        handle_new_window(app, (msg_new_window_t *)msg->data);
        break;
    case WINDOW_RECT:
        handle_window_flush_rect(app, (msg_window_flush_rect_t *)msg->data);
        break;
    case WINDOW_SHOW:
        assert(win);
        handle_window_show(app, win, (msg_show_window_t *)msg->data);
        break;
    case WINDOW_HIDE:
        assert(win);
        handle_window_hide(win);
        break;
    case WINDOW_START_MOVE:
        win_start_move(win, (msg_win_move_t *)msg->data);
        break;
    case WINDOW_END_MOVE:
        win_end_move(win);
        break;
    case WINDOW_MOVE_TO:
    {
        printf("request move from client\n");
        msg_win_move_t *move = (msg_win_move_t *)msg->data;
        win_move_to(win, move->x, move->y);
    }
    break;
    case SYS_MENU:
        win_set_system_menu(win);
        break;
    default:
        printf("unhandled ipc event :: %d\n", msg->event);
        break;
    }
}

void spawn_sysmenu()
{
}

int main(int argc, const char *argv[])
{

    kbd_fd = open("/dev/kbd", O_RDONLY);
    if (kbd_fd == -1)
    {
        perror("open kbd");
        return EXIT_FAILURE;
    }

    mouse_fd = open("/dev/mouse", O_RDONLY);
    if (mouse_fd == -1)
    {
        perror("open mouse");
        return EXIT_FAILURE;
    }

    ipc_fd = open("/dev/ipc/wm", O_CREAT | O_RDWR);
    if (ipc_fd == -1)
    {
        perror("create ipc server");
        return EXIT_FAILURE;
    }

    server_app = calloc(1, sizeof(app_t));
    server_app->app_id = 0;
    server_app->ipc_fd = ipc_fd;
    server_app->running = 1;
    server_app->window_list = NULL;

    init_win();
    gfx = start_compositor();
    assert(gfx);

    get_mouse_pos(&m_x, &m_y);

    {
        int child = fork();
        if (!child)
        {

            const char *args[] = {
                "/bin/files",
                NULL,
            };

            execve(args[0], args, NULL);
            assert(0 && "exec");
        }
    }
    struct pollfd fds[3];
    fds[0].fd = mouse_fd;
    fds[1].fd = ipc_fd;
    fds[2].fd = kbd_fd;

    fds[0].events = POLLIN;
    fds[1].events = POLLIN;
    fds[2].events = POLLIN;

    while (g_running)
    {
        int c = poll(fds, 3, -1);
        if (c == -1)
        {
            perror("poll");
            break;
        }

        int e = 0;
        if (fds[0].revents & POLLIN)
        {
            e++;
            handle_mouse();
        }
        if (fds[1].revents & POLLIN)
        {
            e++;
            handle_ipc();
        }
        if (fds[2].revents & POLLIN)
        {
            e++;
            handle_kbd();
        }

        if (e < c)
        {
            printf("inavalide revents??\n");
            break;
        }
    }

    return 0;
}
