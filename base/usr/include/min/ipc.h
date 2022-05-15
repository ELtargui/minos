#pragma once

#include <stdint.h>
#include <min/gfx.h>

#define MSG_MAGIC 0xc001c0de

typedef struct ipc_msg
{
    uint32_t magic;
    int event;
    uint32_t id;
    int size;
    char data[];
} ipc_msg_t;
