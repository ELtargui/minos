#include <mink/types.h>
#include <mink/i386.h>
#include <mink/process.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/string.h>
#include <mink/fs.h>
#include <mink/ringbuffer.h>
#include <min/mouse.h>

#define ENABLE_AUX_PS2 0xA8
#define GET_COMPAQ_STATUS 0x20
#define SET_COMPAQ_STATUS 0x60
#define USE_DEFAULTS 0xF6
#define ENABLE_MOUSE 0xF4

#define MOUSE_IRQ 12
#define MOUSE_PORT 0x60
#define MOUSE_STATUS 0x64
#define MOUSE_BUFFER_FULL 0x01
#define MOUSE_WHICH_BUFFER 0x20
#define MOUSE_MOUSE_BUFFER 0x20

static void wait_output()
{
    while (1)
    {
        if (!(inportb(MOUSE_STATUS) & 2))
        {
            break;
        }
    }
}
static void wait_input()
{
    while (1)
    {
        if (inportb(MOUSE_STATUS) & 1)
        {
            break;
        }
    }
}

static void get_ack()
{
    wait_input();
    uint8_t ack = inportb(MOUSE_PORT);
    assert(ack == 0xfa);
}
static fsnode_t *mouse;

static uint8_t mouse_data[4];
static int state = 0;

void mouse_packet()
{
    state = 0;

    int x_sign = mouse_data[0] & 0x10;
    int y_sign = mouse_data[0] & 0x20;
    int dx = mouse_data[1];
    int dy = mouse_data[2];

    if (dx && x_sign)
        dx -= 0x100;
    if (dy && y_sign)
        dy -= 0x100;

    if ((mouse_data[0] & 0x40) || (mouse_data[0] & 0x80))
    {
        //overflow discard
        dx = 0;
        dy = 0;
    }

    int btn = 0;

    if (mouse_data[0] & 0x01)
    {
        btn |= MOUSE_LBTN;
    }
    if (mouse_data[0] & 0x02)
    {
        btn |= MOUSE_RBTN;
    }
    if (mouse_data[0] & 0x04)
    {
        btn |= MOUSE_MBTN;
    }

    mouse_packet_t mp;
    mp.magic = MOUSE_MAGIC;
    mp.dx = dx;
    mp.dy = -dy;
    mp.btn = btn;

    while ((uint32_t)rb_size_toread(mouse->self) > 60 * sizeof(mouse_packet_t))
    {
        mouse_packet_t tmp;
        ringbuffer_read(mouse->self, sizeof(mouse_packet_t), &tmp);
    }

    //  dbgln("%d,%d,%d", mp.dx, mp.dy, mp.btn);
    ringbuffer_write(mouse->self, sizeof(mouse_packet_t), &mp);
}

void mouse_handler(regs_t *r)
{
    (void)r;
    interrupt_disable();
    while (1)
    {
        uint8_t status = inportb(MOUSE_STATUS);
        if (!(((status & MOUSE_MOUSE_BUFFER) == MOUSE_MOUSE_BUFFER) && (status & MOUSE_BUFFER_FULL)))
        {
            irq_ack(MOUSE_IRQ);
            interrupt_resume();

            return;
        }

        uint8_t data = inportb(MOUSE_PORT);
        mouse_data[state] = data;
        switch (state)
        {
        case 0:
            if (!(data & 0x08))
            {
                //dbgln("Stream out of sync");
                break;
            }
            state++;
            break;
        case 1:
            state++;
            break;
        case 2:
            state = 3;
            irq_ack(MOUSE_IRQ);
            mouse_packet();
            state = 0;
            interrupt_resume();

            return;
        case 3:
            ERROR(not_finished);
            break;
        }
    }
}

int mouse_read(fsnode_t *node, off_t off, size_t size, void *buf)
{
    (void)off;
    assert(size % sizeof(mouse_packet_t) == 0);
    return ringbuffer_read(node->self, size, buf);
}

int mouse_poll(fsnode_t *node, int events)
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

static fsnode_ops_t mouse_ops = {
    .read = mouse_read,
    .poll = mouse_poll,
};

void mouse_install()
{
    wait_output();
    outportb(MOUSE_STATUS, 0xa8);

    wait_output();
    outportb(MOUSE_STATUS, 0x20);

    wait_input();
    uint8_t status = inportb(MOUSE_PORT);

    wait_output();
    outportb(MOUSE_STATUS, 0x60);

    wait_output();
    outportb(MOUSE_PORT, status | 2);

    wait_output();
    outportb(MOUSE_STATUS, 0xd4);
    wait_output();
    outportb(MOUSE_PORT, 0xf6);
    get_ack();

    wait_output();
    outportb(MOUSE_STATUS, 0xd4);
    wait_output();
    outportb(MOUSE_PORT, 0xf4);
    get_ack();

    // STI();
    mouse = calloc(1, sizeof(fsnode_t));
    strcpy(mouse->name, "mouse");
    mouse->type = FS_CHR;
    mouse->ops = &mouse_ops;

    ringbuffer_t *rb = new_ringbuffer(sizeof(mouse_packet_t) * 64, RB_WRITE_NOBLOCK);
    rb->read_node = mouse;
    mouse->self = rb;

    vfs_bind("/dev/mouse", mouse, 0666);
    install_irq_handler(MOUSE_IRQ, mouse_handler);
    dbgln("mouse installed");
}
