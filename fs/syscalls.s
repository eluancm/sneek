	.section ".text"
	.align	4
	.arm
	.global syscall_00
syscall_00:
	.long 0xe6000010
	bx lr

	.global syscall_01
syscall_01:
	.long 0xe6000030
	bx lr

	.global syscall_02
syscall_02:
	.long 0xe6000050
	bx lr

	.global syscall_03
syscall_03:
	.long 0xe6000070
	bx lr

	.global syscall_04
syscall_04:
	.long 0xe6000090
	bx lr

	.global syscall_05
syscall_05:
	.long 0xe60000b0
	bx lr

	.global syscall_06
syscall_06:
	.long 0xe60000d0
	bx lr

	.global syscall_07
syscall_07:
	.long 0xe60000f0
	bx lr

	.global syscall_08
syscall_08:
	.long 0xe6000110
	bx lr

	.global syscall_09
syscall_09:
	.long 0xe6000130
	bx lr

	.global syscall_0a
syscall_0a:
	.long 0xe6000150
	bx lr

	.global syscall_0b
syscall_0b:
	.long 0xe6000170
	bx lr

	.global syscall_0c
syscall_0c:
	.long 0xe6000190
	bx lr

	.global syscall_0d
syscall_0d:
	.long 0xe60001b0
	bx lr

	.global syscall_0e
syscall_0e:
	.long 0xe60001d0
	bx lr

	.global syscall_0f
syscall_0f:
	.long 0xe60001f0
	bx lr

	.global syscall_10
syscall_10:
	.long 0xe6000210
	bx lr

	.global syscall_11
syscall_11:
	.long 0xe6000230
	bx lr

	.global syscall_12
syscall_12:
	.long 0xe6000250
	bx lr

	.global syscall_13
syscall_13:
	.long 0xe6000270
	bx lr

	.global syscall_14
syscall_14:
	.long 0xe6000290
	bx lr

	.global syscall_15
syscall_15:
	.long 0xe60002b0
	bx lr

	.global syscall_16
syscall_16:
	.long 0xe60002d0
	bx lr

	.global syscall_17
syscall_17:
	.long 0xe60002f0
	bx lr

	.global syscall_18
syscall_18:
	.long 0xe6000310
	bx lr

	.global syscall_19
syscall_19:
	.long 0xe6000330
	bx lr

	.global syscall_1a
syscall_1a:
	.long 0xe6000350
	bx lr

	.global syscall_1b
syscall_1b:
	.long 0xe6000370
	bx lr

	.global syscall_1c
syscall_1c:
	.long 0xe6000390
	bx lr

	.global syscall_1d
syscall_1d:
	.long 0xe60003b0
	bx lr

	.global syscall_1e
syscall_1e:
	.long 0xe60003d0
	bx lr

	.global syscall_1f
syscall_1f:
	.long 0xe60003f0
	bx lr

	.global syscall_20
syscall_20:
	.long 0xe6000410
	bx lr

	.global syscall_21
syscall_21:
	.long 0xe6000430
	bx lr

	.global syscall_22
syscall_22:
	.long 0xe6000450
	bx lr

	.global syscall_23
syscall_23:
	.long 0xe6000470
	bx lr



	.global syscall_2a
syscall_2a:
	.long 0xe6000550
	bx lr


	.global syscall_2f
syscall_2f:
	.long 0xe60005f0
	bx lr

	.global syscall_30
syscall_30:
	.long 0xe6000610
	bx lr

	.global syscall_34
syscall_34:
	.long 0xe6000690
	bx lr

	.global syscall_3f
syscall_3f:
	.long 0xe60007f0
	bx lr

	.global syscall_40
syscall_40:
	.long 0xe6000810
	bx lr

	.global syscall_4b
syscall_4b:
	.long 0xe6000970
	bx lr

	.global syscall_4f
syscall_4f:
	.long 0xe60009f0
	bx lr

	.global syscall_50
syscall_50:
	.long 0xe6000a10
	bx lr

	.global syscall_6b
syscall_6b:
	.long 0xe6000d70
	bx lr

	.global svc_write
svc_write:
	mov r1, r0
	mov r0, #4
	svc #0xab
	bx lr

	.pool
	.end
