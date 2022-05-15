#pragma once

#define MOUSE_MAGIC 0xf11dc0de
#define MOUSE_LBTN 1
#define MOUSE_RBTN 2
#define MOUSE_MBTN 4

typedef struct mouse_packet
{
    unsigned int magic;
    int dx;
    int dy;
    int btn;
} mouse_packet_t;
