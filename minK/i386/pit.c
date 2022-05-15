#include <mink/types.h>
#include <mink/i386.h>
#include <mink/process.h>
#include <mink/debug.h>

#define PIT_FREQUENCY 1193182

static uint32_t pit_hz = 0;
static uint32_t pit_tick = 0;
static uint64_t pit_sec = 0;

void pit_irq_handler(regs_t *r)
{
    (void)r;

    pit_tick++;
    if (pit_tick == pit_hz)
    {
        pit_sec++;
        pit_tick = 0;
        // dbgln("tick[%d]", pit_sec);
    }
    irq_ack(0);
    sched_ontick(pit_sec, pit_tick);
}

uint32_t pit_frequency()
{
    return pit_hz;
}

void start_pit(uint32_t frequency)
{
    pit_hz = frequency;
    uint16_t div = PIT_FREQUENCY / pit_hz;
    outportb(0x43, 0x34);
    outportb(0x40, div & 0xFF);
    outportb(0x40, (div >> 8) & 0xFF);

    install_irq_handler(0, pit_irq_handler);
}
