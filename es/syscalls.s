	.section ".text"
	.align	4
	.arm
	.global thread_create
thread_create:
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

	.global TimerCreate
TimerCreate:
	.long 0xe6000230
	bx lr

	.global TimerRestart
TimerRestart:
	.long 0xe6000250
	bx lr

	.global TimerStop
TimerStop:
	.long 0xe6000270
	bx lr

	.global TimerDestroy
TimerDestroy:
	.long 0xe6000290
	bx lr

	.global TimerNow
TimerNow:
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

	.global IOS_Open
IOS_Open:
	.long 0xe6000390
	bx lr

	.global IOS_Close
IOS_Close:
	.long 0xe60003b0
	bx lr

	.global IOS_Read
IOS_Read:
	.long 0xe60003d0
	bx lr

	.global IOS_Write
IOS_Write:
	.long 0xe60003f0
	bx lr

	.global IOS_Seek
IOS_Seek:
	.long 0xe6000410
	bx lr

	.global IOS_Ioctl
IOS_Ioctl:
	.long 0xe6000430
	bx lr

	.global IOS_Ioctlv
IOS_Ioctlv:
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

	.global syscall_2b
syscall_2b:
	.long 0xe6000570
	bx lr

	.global syscall_2c
syscall_2c:
	.long 0xe6000590
	bx lr

	.global syscall_2d
syscall_2d:
	.long 0xe60005b0
	bx lr

	.global syscall_2e
syscall_2e:
	.long 0xe60005d0
	bx lr

	.global syscall_2f
syscall_2f:
	.long 0xe60005f0
	bx lr

	.global syscall_30
syscall_30:
	.long 0xe6000610
	bx lr

	.global syscall_31
syscall_31:
	.long 0xe6000630
	bx lr

	.global IRQ18
IRQ18:
	.long 0xe6000650
	bx lr

	.global syscall_33
syscall_33:
	.long 0xe6000670
	bx lr

	.global syscall_34
syscall_34:
	.long 0xe6000690
	bx lr

	.global sync_before_read
sync_before_read:
	.long 0xe60007f0
	bx lr

	.global sync_after_write
sync_after_write:
	.long 0xe6000810
	bx lr

	.global syscall_41
syscall_41:
	.long 0xE6000830
	bx lr

	.global syscall_42
syscall_42:
	.long 0xe6000850
	bx lr

	.global syscall_43
syscall_43:
	.long 0xe6000870
	bx lr

	.global DIResetAssert
DIResetAssert:
	.long 0xe6000890
	bx lr

	.global DIResetDeAssert
DIResetDeAssert:
	.long 0xe60008b0
	bx lr

	.global DIResetCheck
DIResetCheck:
	.long 0xe60008d0
	bx lr

	.global syscall_47
syscall_47:
	.long 0xe60008f0
	bx lr
	
	.global syscall_4b
syscall_4b:
	.long 0xe6000970
	bx lr

	.global syscall_4c
syscall_4c:
	.long 0xe6000990
	bx lr

	.global syscall_4d
syscall_4d:
	.long 0xe60009b0
	bx lr

	.global syscall_4e
syscall_4e:
	.long 0xe60009d0
	bx lr

	.global syscall_4f
syscall_4f:
	.long 0xe60009f0
	bx lr

	.global syscall_50
syscall_50:
	.long 0xe6000a10
	bx lr

	.global syscall_51
syscall_51:
	.long 0xe6000a30
	bx lr

	.global EXICtrl
EXICtrl:
	.long 0xe6000a50
	bx lr
	
	.global syscall_54
syscall_54:
	.long 0xe6000a90
	bx lr


	.global syscall_59
syscall_59:
	.long 0xe6000b30
	bx lr

	.global syscall_5a
syscall_5a:
	.long 0xe6000b50
	bx lr

	.global KeyCreate
KeyCreate:
	.long 0xe6000b70
	bx lr

	.global KeyDelete
KeyDelete:
	.long 0xe6000b90
	bx lr

	.global KeyInitialize
KeyInitialize:
	.long 0xe6000bb0
	bx lr

	.global syscall_5f
syscall_5f:
	.long 0xe6000bf0
	bx lr


	.global syscall_61
syscall_61:
	.long 0xe6000c30
	bx lr


	.global GetKey
GetKey:
	.long 0xe6000c70
	bx lr

	.global sha1
sha1:
	.long 0xe6000cf0
	bx lr

	.global syscall_68
syscall_68:
	.long 0xe6000d10
	bx lr

	.global aes_encrypt
aes_encrypt:
	.long 0xe6000d30
	bx lr

	.global syscall_6a
syscall_6a:
	.long 0xe6000d50
	bx lr

	.global syscall_6b
syscall_6b:
	.long 0xe6000d70
	bx lr

	.global syscall_6c
syscall_6c:
	.long 0xe6000d90
	bx lr


	.global syscall_6f
syscall_6f:
	.long 0xe6000df0
	bx lr

	.global GetDeviceCert
GetDeviceCert:
	.long 0xe6000e10
	bx lr

	.global KeySetPermissions
KeySetPermissions:
	.long 0xe6000e30
	bx lr
	
	.global syscall_74
syscall_74:
	.long 0xe6000e90
	bx lr

	.global syscall_75
syscall_75:
	.long 0xe6000eb0
	bx lr

	.global syscall_76
syscall_76:
	.long 0xe6000ed0
	bx lr

	.global svc_write
svc_write:
	mov r1, r0
	mov r0, #4
	svc #0xab
	bx lr

	.pool
	.end
