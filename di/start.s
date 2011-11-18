.global _start
.extern __stack_addr
.extern __bss_start
.extern __bss_end
.extern _main
.arm

	.EQU	ios_thread_arg,			1
	.EQU	ios_thread_priority,	0xF4
	.EQU	ios_thread_stacksize,	0x8000

_start:
	@ setup stack
	mov r2,		#0x8000
	ldr sp,		=0x2022CDC4
	add sp,		sp,				r2

	@ take the plunge
	mov	r0, #0
	mov	r1, #0
	blx	_main

