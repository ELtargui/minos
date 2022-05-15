#pragma once

#include <min/ipc.h>
#include <min/list.h>

typedef struct app
{
    int ipc_fd;
    uint32_t app_id;
    int running;
    int exit_val;
    list_t *window_list;
} app_t;

app_t *new_application(int argc, const char *argv[]);
int app_main(app_t *app);
int app_send_msg_to(app_t *app, uint32_t dst, int type, void *data, size_t size);
int app_send_msg(app_t *app, int type, void *data, size_t size);
int app_recv(app_t *app, void *buf);
