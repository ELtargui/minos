#include <mink/types.h>
#include <mink/multiboot.h>
#include <mink/debug.h>
#include <mink/i386.h>
#include <mink/string.h>

typedef struct gdt_gate
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_gate_t;

typedef struct idt_gate
{
    uint16_t base_low;
    uint16_t sel;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) idt_gate_t;

typedef struct descriptor_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) descriptor_ptr_t;

typedef struct tss
{
    uint32_t prev_tss, esp0, ss0, esp1, ss1, esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs, ldt;

    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

extern unsigned int isr_table[];
extern unsigned int irq_table[];

extern void flush_gdt(void *);
extern void flush_idt(void *);
extern void flush_tss();

static gdt_gate_t GdtGates[6];
static idt_gate_t IdtGates[256];
static tss_t Tss;

void cpu_set_kernel_stack(uintptr_t stack)
{
    Tss.esp0 = stack;
}

void set_gdt_gate(int id, uint32_t B, uint32_t L, uint8_t A, uint8_t G)
{
    GdtGates[id].base_low = B & 0xffff;
    GdtGates[id].base_middle = (B >> 16) & 0xff;
    GdtGates[id].base_high = (B >> 24) & 0xff;

    GdtGates[id].limit_low = L & 0xffff;

    GdtGates[id].access = A;
    GdtGates[id].granularity = ((L >> 16) & 0x0f);
    GdtGates[id].granularity |= (G & 0xf0);
}

void set_idt_gate(int id, uint32_t B, uint16_t S, uint8_t F)
{
    IdtGates[id].base_low = B & 0xffff;
    IdtGates[id].base_high = (B >> 16) & 0xffff;
    IdtGates[id].sel = S;
    IdtGates[id].zero = 0;
    IdtGates[id].flags = F | 0x60;
}

static void install_gdt()
{
    static descriptor_ptr_t ptr;
    ptr.base = (uint32_t)&GdtGates[0];
    ptr.limit = sizeof(GdtGates) - 1;

    set_gdt_gate(0, 0, 0, 0, 0);
    set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // code seg
    set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // data seg
    set_gdt_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // u code seg
    set_gdt_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // u data seg

    memset(&Tss, 0, sizeof(tss_t));
    Tss.ss0 = 0x10;
    Tss.esp0 = 0;
    Tss.cs = 0x0b;
    Tss.ss = 0x13;
    Tss.ds = 0x13;
    Tss.es = 0x13;
    Tss.fs = 0x13;
    Tss.gs = 0x13;
    Tss.iomap_base = sizeof(tss_t);

    set_gdt_gate(5, (uint32_t)&Tss, (uint32_t)&Tss + sizeof(tss_t), 0xE9, 0);

    flush_gdt(&ptr);
    flush_tss();
}

static void install_idt()
{
    memset(IdtGates, 0, sizeof(IdtGates));

    descriptor_ptr_t ptr;
    ptr.base = (uint32_t)&IdtGates[0];
    ptr.limit = sizeof(IdtGates) - 1;

    flush_idt(&ptr);
}

static void install_isr()
{
    for (int i = 0; i < 32; i++)
    {
        set_idt_gate(i, isr_table[i], 0x08, 0x8E);
    }
    set_idt_gate(128, isr_table[32], 0x08, 0x8E);
}

static void install_irq()
{
    outportb(0x20, 0x11);
    outportb(0xa0, 0x11);
    outportb(0x21, 0x20);
    outportb(0xa1, 0x28);
    outportb(0x21, 0x04);
    outportb(0xa1, 0x02);
    outportb(0x21, 0x01);
    outportb(0xa1, 0x01);
    outportb(0x21, 0x00);
    outportb(0xa1, 0x00);

    for (int i = 0; i < 16; i++)
    {
        set_idt_gate(32 + i, irq_table[i], 0x08, 0x8E);
    }
}

void cpu_init()
{
    install_gdt();
    install_idt();
    install_isr();
    install_irq();
    dbgln("cpu initialized.");
}
