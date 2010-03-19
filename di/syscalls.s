	.section ".text"
	.align	4
	.arm
	.global ThreadCreate
ThreadCreate:
	.long 0xe6000010
	bx lr

	.global ThreadJoin
ThreadJoin:
	.long 0xe6000030
	bx lr

	.global ThreadCancel
ThreadCancel:
	.long 0xe6000050
	bx lr

	.global GetProcessID
GetProcessID:
	.long 0xe6000070
	bx lr

	.global GetThreadID
GetThreadID:
	.long 0xe6000090
	bx lr

	.global ThreadContinue
ThreadContinue:
	.long 0xe60000b0
	bx lr

	.global ThreadStop
ThreadStop:
	.long 0xe60000d0
	bx lr

	.global ThreadYield
ThreadYield:
	.long 0xe60000f0
	bx lr

	.global ThreadGetPriority
ThreadGetPriority:
	.long 0xe6000110
	bx lr

	.global ThreadSetPriority
ThreadSetPriority:
	.long 0xe6000130
	bx lr

	.global MessageQueueCreate
MessageQueueCreate:
	.long 0xe6000150
	bx lr

	.global MessageQueueDestroy
MessageQueueDestroy:
	.long 0xe6000170
	bx lr

	.global MessageQueueSend
MessageQueueSend:
	.long 0xe6000190
	bx lr

	.global MessageQueueSendNow
MessageQueueSendNow:
	.long 0xe60001b0
	bx lr

	.global MessageQueueReceive
MessageQueueReceive:
	.long 0xe60001d0
	bx lr

	.global RegisterEventHandler
RegisterEventHandler:
	.long 0xe60001f0
	bx lr

	.global UnregisterEventHandler
UnregisterEventHandler:
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

	.global HeapCreate
HeapCreate:
	.long 0xe60002d0
	bx lr

	.global HeapDestroy
HeapDestroy:
	.long 0xe60002f0
	bx lr

	.global HeapAlloc
HeapAlloc:
	.long 0xe6000310
	bx lr

	.global HeapAllocAligned
HeapAllocAligned:
	.long 0xe6000330
	bx lr

	.global HeapFree
HeapFree:
	.long 0xe6000350
	bx lr

	.global IOS_Register
IOS_Register:
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

	.global IOS_OpenAsync
IOS_OpenAsync:
	.long 0xe6000470
	bx lr

	.global IOS_CloseAsync
IOS_CloseAsync:
	.long 0xe6000490
	bx lr

	.global IOS_ReadAsync
IOS_ReadAsync:
	.long 0xe60004b0
	bx lr

	.global IOS_WriteAsync
IOS_WriteAsync:
	.long 0xe60004d0
	bx lr

	.global IOS_SeekAsync
IOS_SeekAsync:
	.long 0xe60004f0
	bx lr

	.global IOS_IoctlAsync
IOS_IoctlAsync:
	.long 0xe6000510
	bx lr

	.global IOS_IoctlvAsync
IOS_IoctlvAsync:
	.long 0xe6000530
	bx lr

	.global MessageQueueAck
MessageQueueAck:
	.long 0xe6000550
	bx lr

	.global SetUID
SetUID:
	.long 0xe6000570
	bx lr

	.global HMACGetQueueForPID
HMACGetQueueForPID:
	.long 0xe6000590
	bx lr

	.global GetGID
GetGID:
	.long 0xe60005b0
	bx lr

	.global syscall_2e
syscall_2e:
	.long 0xe60005d0
	bx lr

	.global cc_ahbMemFlush
cc_ahbMemFlush:
	.long 0xe60005f0
	bx lr

	.global _ahbMemFlush
_ahbMemFlush:
	.long 0xe6000610
	bx lr

	.global syscall_31
syscall_31:
	.long 0xe6000630
	bx lr

	.global IRQ_Enable_18
IRQ_Enable_18:
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

	.global syscall_35
syscall_35:
	.long 0xe60006b0
	bx lr

	.global syscall_36
syscall_36:
	.long 0xe60006d0
	bx lr

	.global syscall_37
syscall_37:
	.long 0xe60006f0
	bx lr

	.global syscall_38
syscall_38:
	.long 0xe6000710
	bx lr

	.global syscall_39
syscall_39:
	.long 0xe6000730
	bx lr

	.global syscall_3a
syscall_3a:
	.long 0xe6000750
	bx lr

	.global syscall_3b
syscall_3b:
	.long 0xe6000770
	bx lr

	.global syscall_3c
syscall_3c:
	.long 0xe6000790
	bx lr

	.global syscall_3d
syscall_3d:
	.long 0xe60007b0
	bx lr

	.global syscall_3e
syscall_3e:
	.long 0xe60007d0
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
	.long 0xe6000830
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

	.global syscall_48
syscall_48:
	.long 0xe6000910
	bx lr

	.global syscall_49
syscall_49:
	.long 0xe6000930
	bx lr

	.global syscall_4a
syscall_4a:
	.long 0xe6000950
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

	.global VirtualToPhysical
VirtualToPhysical:
	.long 0xe60009f0
	bx lr

	.global EnableVideo
EnableVideo:
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

	.global syscall_53
syscall_53:
	.long 0xe6000a70
	bx lr

	.global syscall_54
syscall_54:
	.long 0xe6000a90
	bx lr

	.global syscall_55
syscall_55:
	.long 0xe6000ab0
	bx lr

	.global syscall_56
syscall_56:
	.long 0xe6000ad0
	bx lr

	.global syscall_57
syscall_57:
	.long 0xe6000af0
	bx lr

	.global syscall_58
syscall_58:
	.long 0xe6000b10
	bx lr

	.global syscall_59
syscall_59:
	.long 0xe6000b30
	bx lr

	.global syscall_5a
syscall_5a:
	.long 0xe6000b50
	bx lr

	.global syscall_5b
syscall_5b:
	.long 0xe6000b70
	bx lr

	.global syscall_5c
syscall_5c:
	.long 0xe6000b90
	bx lr

	.global syscall_5d
syscall_5d:
	.long 0xe6000bb0
	bx lr

	.global syscall_5e
syscall_5e:
	.long 0xe6000bd0
	bx lr

	.global syscall_5f
syscall_5f:
	.long 0xe6000bf0
	bx lr

	.global syscall_60
syscall_60:
	.long 0xe6000c10
	bx lr

	.global syscall_61
syscall_61:
	.long 0xe6000c30
	bx lr

	.global syscall_62
syscall_62:
	.long 0xe6000c50
	bx lr

	.global GetKey
GetKey:
	.long 0xe6000c70
	bx lr

	.global syscall_64
syscall_64:
	.long 0xe6000c90
	bx lr

	.global syscall_65
syscall_65:
	.long 0xe6000cb0
	bx lr

	.global syscall_66
syscall_66:
	.long 0xe6000cd0
	bx lr

	.global AESEncrypt
AESEncrypt:
	.long 0xe6000cf0
	bx lr

	.global syscall_68
syscall_68:
	.long 0xe6000d10
	bx lr

	.global syscall_69
syscall_69:
	.long 0xe6000d30
	bx lr

	.global syscall_6a
syscall_6a:
	.long 0xe6000d50
	bx lr

	.global AESDecrypt
AESDecrypt:
	.long 0xe6000d70
	bx lr

	.global syscall_6c
syscall_6c:
	.long 0xe6000d90
	bx lr

	.global syscall_6d
syscall_6d:
	.long 0xe6000db0
	bx lr

	.global syscall_6e
syscall_6e:
	.long 0xe6000dd0
	bx lr

	.global syscall_6f
syscall_6f:
	.long 0xe6000df0
	bx lr

	.global syscall_70
syscall_70:
	.long 0xe6000e10
	bx lr

	.global syscall_71
syscall_71:
	.long 0xe6000e30
	bx lr

	.global syscall_72
syscall_72:
	.long 0xe6000e50
	bx lr

	.global syscall_73
syscall_73:
	.long 0xe6000e70
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

	.global syscall_77
syscall_77:
	.long 0xe6000ef0
	bx lr

	.global syscall_78
syscall_78:
	.long 0xe6000f10
	bx lr

	.global syscall_79
syscall_79:
	.long 0xe6000f30
	bx lr

	.global syscall_7a
syscall_7a:
	.long 0xe6000f50
	bx lr

	.global syscall_7b
syscall_7b:
	.long 0xe6000f70
	bx lr

	.global syscall_7c
syscall_7c:
	.long 0xe6000f90
	bx lr

	.global syscall_7d
syscall_7d:
	.long 0xe6000fb0
	bx lr

	.global syscall_7e
syscall_7e:
	.long 0xe6000fd0
	bx lr

	.global syscall_7f
syscall_7f:
	.long 0xe6000ff0
	bx lr

	.global OSReport
OSReport:
	mov r1, r0
	mov r0, #4
	svc #0xab
	bx lr

	.pool
	.end
