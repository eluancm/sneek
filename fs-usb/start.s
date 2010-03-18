.global _start
.extern _main
.arm

_start:
	@ setup stack
	mov r2,		#0x800
	ldr sp,		=0x2004F1EC
	add sp,		sp,				r2

	@ take the plunge
	mov	r0, #0
	mov	r1, #0
	blx	_main
