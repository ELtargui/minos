.section .text

.global flush_gdt
.type flush_gdt, @function

.global flush_tss
.type flush_tss, @function

.global flush_idt
.type flush_idt, @function

.global read_eip
.type read_eip, @function

flush_gdt:
    mov 4(%esp), %eax
    lgdt (%eax)

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %ss

    ljmp $0x08, $.flush
.flush:
    ret

flush_tss:
    mov $0x2B, %ax
    ltr %ax
    ret

flush_idt:
    mov 4(%esp), %eax
    lidt (%eax)
    ret

read_eip:
    mov (%esp), %eax
    ret


.type enter_user_space, @function
.global enter_user_space

enter_user_space:
    pushl %ebp
    mov %esp, %ebp
    mov 0xC(%ebp), %edx
    mov %edx, %esp
    pushl $0x12345678
    /* Segement selector */
    mov $0x23,%ax
    /* Save segement registers */
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs
    /* %ss is handled by iret */
    /* Store stack address in %eax */
    mov %esp, %eax
    /* Data segmenet with bottom 2 bits set for ring3 */
    pushl $0x23
    /* Push the stack address */
    pushl %eax
    /* Push flags and fix interrupt flag */
    pushf
    popl %eax
    /* Request ring3 */
    orl $0x200, %eax
    pushl %eax
    pushl $0x1B
    /* Push entry point */
    pushl 0x8(%ebp)
    iret
    popl %ebp
    ret

.type return_to_user, @function
.global return_to_user
return_to_user:
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret


.macro ISR_NOERR index
    .global _isr\index
    _isr\index:
        cli
        push $0
        push $\index
        jmp isrCommon
.endm

.macro ISR_ERR index
    .global _isr\index
    _isr\index:
        cli
        push $\index
        jmp isrCommon
.endm

.macro IRQ ident byte
    .global _irq\ident
    .type _irq\ident, @function
    _irq\ident:
        cli
        push $0x00
        push $\byte
        jmp irqCommon
.endm

.macro ISRN n
    .long _isr\n
.endm

.macro IRQN n
    .long _irq\n
.endm


ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31
ISR_NOERR 128

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

.global irq_table
.global isr_table

irq_table:
    IRQN 0
    IRQN 1
    IRQN 2
    IRQN 3
    IRQN 4
    IRQN 5
    IRQN 6
    IRQN 7
    IRQN 8
    IRQN 9
    IRQN 10
    IRQN 11
    IRQN 12
    IRQN 13
    IRQN 14
    IRQN 15

isr_table:
    ISRN 0
    ISRN 1
    ISRN 2
    ISRN 3
    ISRN 4
    ISRN 5
    ISRN 6
    ISRN 7
    ISRN 8
    ISRN 9
    ISRN 10
    ISRN 11
    ISRN 12
    ISRN 13
    ISRN 14
    ISRN 15
    ISRN 16
    ISRN 17
    ISRN 18
    ISRN 19
    ISRN 20
    ISRN 21
    ISRN 22
    ISRN 23
    ISRN 24
    ISRN 25
    ISRN 26
    ISRN 27
    ISRN 28
    ISRN 29
    ISRN 30
    ISRN 31
    ISRN 128


.extern isr_handler
.type isr_handler, @function
.global isrCommon

isrCommon:
    pusha

    push %ds
    push %es
    push %fs
    push %gs

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    cld
    push %esp

    call isr_handler
    add $4, %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds

    popa
    add $8, %esp
    iret

.extern irq_handler
.type irq_handler, @function
.global irqCommon

irqCommon:
    pusha

    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    cld
    push %esp

    call irq_handler
    add $4, %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds

    popa
    add $8, %esp
    iret
