#pragma once

#include <mink/types.h>
#include <mink/debug.h>
#include <mink/fs.h>
#include <mink/lock.h>

#define RB_READ_CLOSED (1 << 0)
#define RB_WRITE_CLOSED (1 << 1)

#define RB_READ_NOBLOCK (1 << 2)
#define RB_WRITE_NOBLOCK (1 << 3)

#define RB_CLOSED (RB_READ_CLOSED | RB_WRITE_CLOSED)
#define RB_NOBLOCK (RB_READ_NOBLOCK | RB_WRITE_NOBLOCK)

typedef struct ringbuffer
{
    int size;
    uint8_t *buffer;
    int wpos;
    int rpos;
    int flags;
    lock_t lock;

    list_t *read_queue;
    list_t *write_queue;
    fsnode_t *read_node;
    fsnode_t *write_node;
} ringbuffer_t;

ringbuffer_t *new_ringbuffer(size_t bufsize, int flags);
void free_ringbuffer(ringbuffer_t *rb);
int rb_size_toread(ringbuffer_t *rb);
int rb_size_towrite(ringbuffer_t *rb);
int ringbuffer_read(ringbuffer_t *rb, size_t size, void *buf);
int ringbuffer_write(ringbuffer_t *rb, size_t size, void *buf);
