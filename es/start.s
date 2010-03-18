.global _start
.extern __stack_addr
.extern __bss_start
.extern __bss_end
.extern _main
.arm

	.EQU	ios_thread_arg,			5
	.EQU	ios_thread_priority,	121
	.EQU	ios_thread_stacksize,	16384

_start:
	@ setup stack
	mov r2,		#0x4000
	ldr sp,		=0x2010D5A0
	add sp,		sp,				r2

	mov	r5,	r0
	mov	r6,	lr
	
	@ take the plunge
	mov		r0,					r5
	mov		lr,					r6
	mov		r1,					#0
	blx		_main

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

