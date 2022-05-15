.section .multiboot
.align 4

.set ALIGN,			1 << 0
.set MEM,			1 << 1
.set GFX,			1 << 2
.set FLAGS,	ALIGN | MEM | GFX
.set MBMAGIG, 0x1BADB002
.set CHEKSUM, -(MBMAGIG + FLAGS)

.long MBMAGIG    		
.long FLAGS       	
.long CHEKSUM       	
.long 0                         
.long 0                         
.long 0                         
.long 0                         
.long 0        

    /**  vbe  */
.long 0                         /* type */
.long 800                         /* width */
.long 600                         /* height */
.long 32                         /* depth */


.section .text
.align 4

.global _start

.extern main
.type main, @function

_start:
	cli
	cld
	
	mov $init_stack, %esp
	and $-16, %esp

	push %esp
	push %ebx
	push %eax
	
	xor %ebp, %ebp
	call main
hang:
	hlt
	jmp hang

.section .stack
.align 4
init_stack_bot:
	.skip 0x8000
init_stack:
