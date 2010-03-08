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

	.section ".ios_info_table","ax",%progbits
	.global ios_info_table
ios_info_table:
	.long	0x0
	.long	0x28		@ numentries * 0x28
	.long	0x6	

	.long	0xB
	.long	ios_thread_arg	@ passed to thread entry func, maybe module id

	.long	0x9
	.long	_start

	.long	0x7D
	.long	ios_thread_priority

	.long	0x7E
	.long	ios_thread_stacksize

	.long	0x7F
	.long	0x2004f1c4

