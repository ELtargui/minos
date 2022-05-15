#pragma once

#include <mink/types.h>

#define CLI() asm volatile("cli\n")
#define STI() asm volatile("sti\n")
#define HLT() asm volatile("hlt\n")

typedef struct regs
{
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t interrupt;
    uint32_t error;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t uesp;
    uint32_t ss;
} __attribute__((packed)) regs_t;

typedef struct vmpage
{
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t writethrough : 1;
    uint32_t cachedisable : 1;
    uint32_t __unused : 7;
    uint32_t frame : 20;
} vmpage_t;

typedef struct vmtable
{
    vmpage_t pages[1024];
} vmtable_t;

typedef struct vmdir
{
    uint32_t ptrs[1024];
    vmtable_t *tables[1024];
} vmdir_t;

typedef void (*interrupt_handler_t)(regs_t *);

extern uintptr_t read_eip(void);
extern void enter_user_space(uintptr_t eip, uintptr_t esp);
extern void return_to_user(void);

uint8_t inportb(uint16_t port);
void outportb(uint16_t port, uint8_t data);
uint16_t inportw(uint16_t port);
void outportw(uint16_t port, uint16_t data);
uint32_t inportd(uint16_t port);
void outportd(uint16_t port, uint32_t data);

void cpu_init();
void cpu_set_kernel_stack(uintptr_t stack);

void interrupt_enable();
void interrupt_disable();
int interrupt_state();
int interrupt_resume();

void install_isr_handler(int isr, interrupt_handler_t handler);
void install_irq_handler(int irq, interrupt_handler_t handler);

void irq_ack(int irq);
uint32_t pit_frequency();
void start_pit(uint32_t frequency);

extern vmdir_t *kernel_vmdir;
uintptr_t vm_paddr(vmdir_t *dir, uintptr_t vaddr);
vmpage_t *get_vmpage(vmdir_t *dir, uintptr_t vaddr);
vmpage_t *make_vmpage(vmdir_t *dir, uintptr_t vaddr);
void vm_free_page(vmpage_t *page);

uint32_t vm_alloc_page(vmpage_t *page, int u, int rw);
void vm_map_page(vmpage_t *page, int u, int rw, uint32_t paddr);
vmdir_t *vm_new_vmdir();

void vm_flush_vmdir(vmdir_t *dir);
void inavalidate_page(uint32_t vaddr);
void inavalidate_region(uint32_t vaddr, uint32_t size);
void inavalidate_all();
