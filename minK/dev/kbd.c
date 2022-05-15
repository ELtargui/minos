#include <mink/types.h>
#include <mink/i386.h>
#include <mink/process.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/string.h>
#include <mink/fs.h>
#include <mink/ringbuffer.h>

static fsnode_t *kbd;

int kbd_read(fsnode_t *node, off_t off, size_t size, void *buf)
{
    (void)off;
    return ringbuffer_read(node->self, size, buf);
}

int kbd_poll(fsnode_t *node, int events)
{
    if (events & POLLIN)
    {
        int e = rb_size_toread(node->self);
        if (e > 0)
            return POLLIN;
    }

    if (events & POLLOUT)
    {
        if (rb_size_towrite(node->self) > 0)
            return POLLOUT;
    }

    return 0;
}

fsnode_ops_t kbd_ops = {
    .read = kbd_read,
    .poll = kbd_poll,
};

void kbd_handler(regs_t *r)
{
    (void)r;
    if (inportb(0x64) & 0x01)
    {
        uint8_t scancode = inportb(0x60);
        ringbuffer_write(kbd->self, 1, &scancode);
        if (scancode == 1)
        {
            sched_debug_threads();
        }
    }
    irq_ack(1);
}

void kbd_install()
{
    kbd = calloc(1, sizeof(fsnode_t));
    strcpy(kbd->name, "kbd");
    kbd->type = FS_CHR;
    kbd->ops = &kbd_ops;
    ringbuffer_t *rb = new_ringbuffer(128, RB_WRITE_NOBLOCK);
    rb->read_node = kbd;
    kbd->self = rb;

    vfs_bind("/dev/kbd", kbd, 0666);
    install_irq_handler(1, kbd_handler);
    dbgln("kbd installed");
}
