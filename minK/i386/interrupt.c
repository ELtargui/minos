#include <mink/types.h>
#include <mink/i386.h>
#include <mink/debug.h>

static interrupt_handler_t isr_handlers_table[256] = {NULL};
static interrupt_handler_t irq_handlers_table[16] = {NULL};

static const char *cpu_exception[32] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Detected overflow",
    "Out-of-bounds",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unknown interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"};

void irq_handler(regs_t *r)
{
    if (r->interrupt <= 47 && r->interrupt >= 32)
    {
        if (irq_handlers_table[r->interrupt - 32])
        {
            interrupt_disable();
            irq_handlers_table[r->interrupt - 32](r);
            interrupt_resume();
        }
        else
        {
            dbgln("irq [%d] no handler installed", r->interrupt - 32);
        }
    }
    else
    {
        assert(0);
        CLI();
        HLT();
    }
}

void irq_ack(int irq)
{
    if (irq > 8)
    {
        outportb(0xA0, 0x20);
    }
    outportb(0x20, 0x20);
}

void isr_handler(regs_t *r)
{
    if (r->interrupt < 256 && isr_handlers_table[r->interrupt])
    {
        if (r->interrupt != 128)
        {
            interrupt_disable();
            isr_handlers_table[r->interrupt](r);
            interrupt_resume();
        }
        else
        {
            isr_handlers_table[r->interrupt](r);
        }
    }
    else
    {
        if (r->interrupt < 32)
        {
            dbgln("CPU exception [%s]", cpu_exception[r->interrupt]);

            dbgln("esp:%p ebp:%p eip:%p", r->esp, r->ebp, r->eip);
        }
        CLI();
        assert(0);
        HLT();
    }
}

void install_isr_handler(int isr, interrupt_handler_t handler)
{
    isr_handlers_table[isr] = handler;
}

void install_irq_handler(int irq, interrupt_handler_t handler)
{
    irq_handlers_table[irq] = handler;
}

uint32_t read_eflags()
{
    uint32_t flags;
    asm volatile(
        "pushf\n"
        "pop %0\n"
        : "=rm"(flags)::"memory");
    return flags;
}

static int int_state = 0;
void interrupt_enable()
{
    int_state = 0;
    STI();
}

void interrupt_disable()
{
    int enabled = read_eflags() & 0x200;
    if (enabled && int_state)
    {
        dbgln("alredy enabled");
        int_state = 0;
    }
    CLI();
    int_state++;
    // dbgln("disable : %d", int_state);
}

int interrupt_state()
{
    return int_state;
}

int interrupt_resume()
{
    int enabled = read_eflags() & 0x200;
    if (enabled)
    {
        // dbgln("alredy enabled");
        int_state = 0;
    }

    int_state--;
    if (int_state <= 0)
    {
        interrupt_enable();
    }
    // dbgln("resume : %d", int_state);

    return int_state;
}
