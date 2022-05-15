#include <mink/types.h>
#include <mink/i386.h>
#include <mink/process.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/string.h>
#include <mink/fs.h>
#include <mink/fsevent.h>
#include <mink/lock.h>
#include <mink/ringbuffer.h>
#include <errno.h>
#include <mink/stdio.h>

ringbuffer_t *new_ringbuffer(size_t bufsize, int flags)
{
    ringbuffer_t *rb = calloc(1, sizeof(ringbuffer_t));
    rb->size = bufsize;
    rb->buffer = calloc(1, bufsize);
    rb->flags = flags;
    rb->read_queue = new_list();
    rb->write_queue = new_list();

    char name[32];
    sprintf(name, "rb:%p:rq", rb);
    rb->read_queue->name = strdup(name);
    sprintf(name, "rb:%p:wq", rb);
    rb->write_queue->name = strdup(name);

    init_lock(&rb->lock, "ringbuffer");

    return rb;
}

void free_ringbuffer(ringbuffer_t *rb)
{
    rb->flags = RB_WRITE_CLOSED | RB_READ_CLOSED;
    sched_run_queue(rb->read_queue, -EPIPE);
    sched_run_queue(rb->write_queue, -EPIPE);

    free(rb->buffer);
    free(rb->read_queue);
    free(rb->write_queue);
    free(rb);
}

int rb_size_toread(ringbuffer_t *rb)
{
    assert(rb);
    if (rb->flags & RB_READ_CLOSED)
        return -1;

    if (rb->rpos < rb->wpos)
    {
        return rb->wpos - rb->rpos;
    }

    if (rb->wpos < rb->rpos)
    {
        return rb->size - rb->rpos + rb->wpos;
    }

    return 0;
}

int rb_size_towrite(ringbuffer_t *rb)
{
    if (rb->flags & RB_WRITE_CLOSED)
        return -1;

    if (rb->wpos < rb->rpos)
    {
        return rb->rpos - rb->wpos;
    }

    if (rb->rpos <= rb->wpos)
    {
        return rb->size - rb->wpos + rb->rpos;
    }

    return 0;
}

int ringbuffer_read(ringbuffer_t *rb, size_t size, void *buf)
{
    if (rb->flags & RB_READ_CLOSED)
    {
        dbgln("rb %p read closed", rb);
        return -EPIPE;
    }

    int ret = 0;
    while (ret == 0)
    {

        int avialable = rb_size_toread(rb);
        assert((int)avialable != -1);

        if (avialable)
        {
            uint8_t *b = buf;

            if (avialable > (int)size)
                avialable = size;

            for (int i = 0; i < avialable; i++)
            {
                b[ret++] = rb->buffer[rb->rpos++];
                if (rb->rpos >= rb->size)
                    rb->rpos = 0;
            }
        }
        else
        {
            //weakup writers
            sched_run_queue(rb->write_queue, 0);
            if (rb->write_node)
                fsnode_event(rb->write_node, POLLOUT);

            if (rb->flags & RB_READ_NOBLOCK)
                return ret;

            // dbgln("block");
            int e = sched_block_on(rb->read_queue, THREAD_BLOCKED);
            if (e)
            {
                dbgln("rb:%p interrupted :%d", rb, e);
                return e;
            }
        }
    }

    if (ret)
    {
        sched_run_queue(rb->write_queue, 0);
        if (rb->write_node)
            fsnode_event(rb->write_node, POLLOUT);
    }

    return ret;
}

int ringbuffer_write(ringbuffer_t *rb, size_t size, void *buf)
{
    if (rb->flags & RB_WRITE_CLOSED)
    {
        dbgln("rb %p write closed", rb);
        return -EPIPE;
    }

    size_t sz = 0;

    while (sz < size)
    {
        size_t avialable = rb_size_towrite(rb);
        assert((int)avialable != -1);
        if (avialable)
        {
            if (avialable > size)
                avialable = size;
            uint8_t *b = buf;
            while (avialable--)
            {
                rb->buffer[rb->wpos++] = b[sz++];
                if (rb->wpos >= rb->size)
                    rb->wpos = 0;
            }
        }

        if (sz < size)
        {
            //weakup readers
            sched_run_queue(rb->read_queue, 0);
            if (rb->read_node)
                fsnode_event(rb->read_node, POLLIN);

            if (rb->flags & RB_WRITE_NOBLOCK)
                return sz;

            dbgln("block");
            int e = sched_block_on(rb->write_queue, THREAD_BLOCKED);
            if (e)
            {
                dbgln("rb:%p interrupted :%d", rb, e);
                return e;
            }
        }
    }

    sched_run_queue(rb->read_queue, 0);
    if (rb->read_node)
        fsnode_event(rb->read_node, POLLIN);

    return sz;
}
