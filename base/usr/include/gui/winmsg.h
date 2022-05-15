#pragma once
#include <stdint.h>

typedef enum ipc_event
{
    APP_CONNECT,
    APP_DISCONNECTED,
    APP_HELLO,
    APP_QUIT,
    WIN_EXIT,
    NEW_WINDOW,
    WINDOW_SET_INFO,
    WINDOW_RECT,
    WINDOW_SHOW,
    USER_MSG,
    MOUSE_CLICK,
    MOUSE_RELEASE,
    MOUSE_MOVE,
    WIN_ENTER,
    WIN_LEAVE,
} ipc_event_t;

typedef struct msg_head
{
    uint32_t winid;
    char data[];
} msg_head_t;

typedef struct msg_new_window
{
    uint32_t winid;
    rect_t rect;
    int frameless;
} msg_new_window_t;

typedef struct msg_show_window
{
    uint32_t winid;
    rect_t rect;
    int frameless;
} msg_show_window_t;

typedef struct msg_window_buffer
{
    uint32_t winid;
    rect_t rect;
    char shared_buffer[64];
} msg_window_info_t;

typedef struct msg_window_flush_rect
{
    uint32_t winid;
    rect_t rect;
} msg_window_flush_rect_t;

typedef struct msg_mouse_event
{
    uint32_t winid;
    int x;
    int y;
    int btn;
} msg_mouse_event_t;
