#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <gui/app.h>
#include <gui/winmsg.h>
#include <gui/window.h>

extern void win_handle_msg(app_t *app, ipc_msg_t *msg);

static const char *program_name(const char *path)
{
    const char *p = path;
    const char *name = p + 1;
    while (*p)
    {
        if (*p == '/' && p[1])
            name = p + 1;
        p++;
    }
    return name;
}

app_t *new_application(int argc, const char *argv[])
{
    char tmp[512];

    sprintf(tmp, "/dev/ipc/wm.%d_%s", getpid(), program_name(argv[0]));
    int ipc = open(tmp, O_RDWR | O_CREAT, 0666);
    if (ipc == -1)
    {
        perror("create connection");
        return NULL;
    }

    app_t *app = calloc(1, sizeof(app_t));
    app->ipc_fd = ipc;

    if (app_send_msg(app, APP_CONNECT, NULL, 0) == -1)
    {
        assert(0);
    }

    int msg = app_recv(app, tmp);
    if (msg != APP_HELLO)
    {
        assert(0);
    }

    printf("connected\n");
    app->running = 1;
    app->window_list = new_list();
    return app;
}

void app_add_window(app_t *app, window_t *win)
{
    if (app->window_list->head == NULL)
    {
        //init fonts
    }

    list_append(app->window_list, win);
}

int app_main(app_t *app)
{
    while (app->running)
    {
        char tmp[512];
        int mtype = app_recv(app, tmp);
        win_handle_msg(app, (ipc_msg_t *)tmp);
        if (mtype == -1)
            break;
    }
    return app->exit_val;
}

int app_send_msg_to(app_t *app, uint32_t dst, int type, void *data, size_t size)
{
    char tmp[512];
    ipc_msg_t *msg = (ipc_msg_t *)tmp;
    msg->event = type;
    msg->magic = MSG_MAGIC;
    msg->id = dst;
    msg->magic = MSG_MAGIC;
    msg->size = size;
    if (data)
        memcpy(msg->data, data, size);

    int e = write(app->ipc_fd, msg, sizeof(ipc_msg_t) + size);
    if (e == -1)
    {
        perror("send msg");
        return -1;
    }
    return 0;
}

int app_send_msg(app_t *app, int type, void *data, size_t size)
{
    return app_send_msg_to(app, 0, type, data, size);
}

int app_recv(app_t *app, void *buf)
{
    ssize_t size = read(app->ipc_fd, buf, 512);
    if (size == -1)
    {
        perror("recv");
        return -1;
    }

    ipc_msg_t *msg = (ipc_msg_t *)buf;
    
    return msg->event;
}
