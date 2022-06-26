#pragma once
#include <stdint.h>
#include <gfx/rect.h>

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
    WINDOW_HIDE,
    WINDOW_SET_VISIBLE,

    MOUSE_CLICK,
    MOUSE_RELEASE,
    MOUSE_MOVE,

    WINDOW_START_MOVE,
    WINDOW_END_MOVE,
    WINDOW_MOVE_TO,
    WINDOW_POSITION,
    
    WIN_DIALOG_WINDOW,
    WIN_SHOW_MENU,
    WIN_HIDE_MENU,

    WIN_ENTER,
    WIN_LEAVE,

    SYS_MENU,
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

typedef struct msg_win_move
{
    uint32_t winid;
    int x;
    int y;
} msg_win_move_t;

typedef struct msg_win_position
{
    uint32_t winid;
    int x;
    int y;
} msg_win_position_t;

typedef struct msg_set_value
{
    uint32_t winid;
    union{
        int value_int;
    };
}msg_set_value_t;

typedef struct msg_sys_menu
{
    uint32_t accepted;
    int error;
}msg_sys_menu_t;
