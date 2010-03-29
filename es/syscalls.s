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

	.global syscall_24
syscall_24:
	.long 0xe6000490
	bx lr

	.global syscall_25
syscall_25:
	.long 0xe60004b0
	bx lr

	.global syscall_26
syscall_26:
	.long 0xe60004d0
	bx lr

	.global syscall_27
syscall_27:
	.long 0xe60004f0
	bx lr

	.global syscall_28
syscall_28:
	.long 0xe6000510
	bx lr

	.global syscall_29
syscall_29:
	.long 0xe6000530
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

	.global CreateKey
CreateKey:
	.long 0xe6000b70
	bx lr

	.global DestroyKey
DestroyKey:
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

	.global GetDeviceCert
GetDeviceCert:
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

	.global syscall_80
syscall_80:
	.long 0xe6001010
	bx lr

	.global syscall_81
syscall_81:
	.long 0xe6001030
	bx lr

	.global syscall_82
syscall_82:
	.long 0xe6001050
	bx lr

	.global syscall_83
syscall_83:
	.long 0xe6001070
	bx lr

	.global syscall_84
syscall_84:
	.long 0xe6001090
	bx lr

	.global syscall_85
syscall_85:
	.long 0xe60010b0
	bx lr

	.global syscall_86
syscall_86:
	.long 0xe60010d0
	bx lr

	.global syscall_87
syscall_87:
	.long 0xe60010f0
	bx lr

	.global syscall_88
syscall_88:
	.long 0xe6001110
	bx lr

	.global syscall_89
syscall_89:
	.long 0xe6001130
	bx lr

	.global syscall_8a
syscall_8a:
	.long 0xe6001150
	bx lr

	.global syscall_8b
syscall_8b:
	.long 0xe6001170
	bx lr

	.global syscall_8c
syscall_8c:
	.long 0xe6001190
	bx lr

	.global syscall_8d
syscall_8d:
	.long 0xe60011b0
	bx lr

	.global syscall_8e
syscall_8e:
	.long 0xe60011d0
	bx lr

	.global syscall_8f
syscall_8f:
	.long 0xe60011f0
	bx lr

	.global syscall_90
syscall_90:
	.long 0xe6001210
	bx lr

	.global syscall_91
syscall_91:
	.long 0xe6001230
	bx lr

	.global syscall_92
syscall_92:
	.long 0xe6001250
	bx lr

	.global syscall_93
syscall_93:
	.long 0xe6001270
	bx lr

	.global syscall_94
syscall_94:
	.long 0xe6001290
	bx lr

	.global syscall_95
syscall_95:
	.long 0xe60012b0
	bx lr

	.global syscall_96
syscall_96:
	.long 0xe60012d0
	bx lr

	.global syscall_97
syscall_97:
	.long 0xe60012f0
	bx lr

	.global syscall_98
syscall_98:
	.long 0xe6001310
	bx lr

	.global syscall_99
syscall_99:
	.long 0xe6001330
	bx lr

	.global syscall_9a
syscall_9a:
	.long 0xe6001350
	bx lr

	.global syscall_9b
syscall_9b:
	.long 0xe6001370
	bx lr

	.global syscall_9c
syscall_9c:
	.long 0xe6001390
	bx lr

	.global syscall_9d
syscall_9d:
	.long 0xe60013b0
	bx lr

	.global syscall_9e
syscall_9e:
	.long 0xe60013d0
	bx lr

	.global syscall_9f
syscall_9f:
	.long 0xe60013f0
	bx lr

	.global syscall_a0
syscall_a0:
	.long 0xe6001410
	bx lr

	.global syscall_a1
syscall_a1:
	.long 0xe6001430
	bx lr

	.global syscall_a2
syscall_a2:
	.long 0xe6001450
	bx lr

	.global syscall_a3
syscall_a3:
	.long 0xe6001470
	bx lr

	.global syscall_a4
syscall_a4:
	.long 0xe6001490
	bx lr

	.global syscall_a5
syscall_a5:
	.long 0xe60014b0
	bx lr

	.global syscall_a6
syscall_a6:
	.long 0xe60014d0
	bx lr

	.global syscall_a7
syscall_a7:
	.long 0xe60014f0
	bx lr

	.global syscall_a8
syscall_a8:
	.long 0xe6001510
	bx lr

	.global syscall_a9
syscall_a9:
	.long 0xe6001530
	bx lr

	.global syscall_aa
syscall_aa:
	.long 0xe6001550
	bx lr

	.global syscall_ab
syscall_ab:
	.long 0xe6001570
	bx lr

	.global syscall_ac
syscall_ac:
	.long 0xe6001590
	bx lr

	.global syscall_ad
syscall_ad:
	.long 0xe60015b0
	bx lr

	.global syscall_ae
syscall_ae:
	.long 0xe60015d0
	bx lr

	.global syscall_af
syscall_af:
	.long 0xe60015f0
	bx lr

	.global syscall_b0
syscall_b0:
	.long 0xe6001610
	bx lr

	.global syscall_b1
syscall_b1:
	.long 0xe6001630
	bx lr

	.global syscall_b2
syscall_b2:
	.long 0xe6001650
	bx lr

	.global syscall_b3
syscall_b3:
	.long 0xe6001670
	bx lr

	.global syscall_b4
syscall_b4:
	.long 0xe6001690
	bx lr

	.global syscall_b5
syscall_b5:
	.long 0xe60016b0
	bx lr

	.global syscall_b6
syscall_b6:
	.long 0xe60016d0
	bx lr

	.global syscall_b7
syscall_b7:
	.long 0xe60016f0
	bx lr

	.global syscall_b8
syscall_b8:
	.long 0xe6001710
	bx lr

	.global syscall_b9
syscall_b9:
	.long 0xe6001730
	bx lr

	.global syscall_ba
syscall_ba:
	.long 0xe6001750
	bx lr

	.global syscall_bb
syscall_bb:
	.long 0xe6001770
	bx lr

	.global syscall_bc
syscall_bc:
	.long 0xe6001790
	bx lr

	.global syscall_bd
syscall_bd:
	.long 0xe60017b0
	bx lr

	.global syscall_be
syscall_be:
	.long 0xe60017d0
	bx lr

	.global syscall_bf
syscall_bf:
	.long 0xe60017f0
	bx lr

	.global syscall_c0
syscall_c0:
	.long 0xe6001810
	bx lr

	.global syscall_c1
syscall_c1:
	.long 0xe6001830
	bx lr

	.global syscall_c2
syscall_c2:
	.long 0xe6001850
	bx lr

	.global syscall_c3
syscall_c3:
	.long 0xe6001870
	bx lr

	.global syscall_c4
syscall_c4:
	.long 0xe6001890
	bx lr

	.global syscall_c5
syscall_c5:
	.long 0xe60018b0
	bx lr

	.global syscall_c6
syscall_c6:
	.long 0xe60018d0
	bx lr

	.global syscall_c7
syscall_c7:
	.long 0xe60018f0
	bx lr

	.global syscall_c8
syscall_c8:
	.long 0xe6001910
	bx lr

	.global syscall_c9
syscall_c9:
	.long 0xe6001930
	bx lr

	.global syscall_ca
syscall_ca:
	.long 0xe6001950
	bx lr

	.global syscall_cb
syscall_cb:
	.long 0xe6001970
	bx lr

	.global syscall_cc
syscall_cc:
	.long 0xe6001990
	bx lr

	.global syscall_cd
syscall_cd:
	.long 0xe60019b0
	bx lr

	.global syscall_ce
syscall_ce:
	.long 0xe60019d0
	bx lr

	.global syscall_cf
syscall_cf:
	.long 0xe60019f0
	bx lr

	.global syscall_d0
syscall_d0:
	.long 0xe6001a10
	bx lr

	.global syscall_d1
syscall_d1:
	.long 0xe6001a30
	bx lr

	.global syscall_d2
syscall_d2:
	.long 0xe6001a50
	bx lr

	.global syscall_d3
syscall_d3:
	.long 0xe6001a70
	bx lr

	.global syscall_d4
syscall_d4:
	.long 0xe6001a90
	bx lr

	.global syscall_d5
syscall_d5:
	.long 0xe6001ab0
	bx lr

	.global syscall_d6
syscall_d6:
	.long 0xe6001ad0
	bx lr

	.global syscall_d7
syscall_d7:
	.long 0xe6001af0
	bx lr

	.global syscall_d8
syscall_d8:
	.long 0xe6001b10
	bx lr

	.global syscall_d9
syscall_d9:
	.long 0xe6001b30
	bx lr

	.global syscall_da
syscall_da:
	.long 0xe6001b50
	bx lr

	.global syscall_db
syscall_db:
	.long 0xe6001b70
	bx lr

	.global syscall_dc
syscall_dc:
	.long 0xe6001b90
	bx lr

	.global syscall_dd
syscall_dd:
	.long 0xe6001bb0
	bx lr

	.global syscall_de
syscall_de:
	.long 0xe6001bd0
	bx lr

	.global syscall_df
syscall_df:
	.long 0xe6001bf0
	bx lr

	.global syscall_e0
syscall_e0:
	.long 0xe6001c10
	bx lr

	.global syscall_e1
syscall_e1:
	.long 0xe6001c30
	bx lr

	.global syscall_e2
syscall_e2:
	.long 0xe6001c50
	bx lr

	.global syscall_e3
syscall_e3:
	.long 0xe6001c70
	bx lr

	.global syscall_e4
syscall_e4:
	.long 0xe6001c90
	bx lr

	.global syscall_e5
syscall_e5:
	.long 0xe6001cb0
	bx lr

	.global syscall_e6
syscall_e6:
	.long 0xe6001cd0
	bx lr

	.global syscall_e7
syscall_e7:
	.long 0xe6001cf0
	bx lr

	.global syscall_e8
syscall_e8:
	.long 0xe6001d10
	bx lr

	.global syscall_e9
syscall_e9:
	.long 0xe6001d30
	bx lr

	.global syscall_ea
syscall_ea:
	.long 0xe6001d50
	bx lr

	.global syscall_eb
syscall_eb:
	.long 0xe6001d70
	bx lr

	.global syscall_ec
syscall_ec:
	.long 0xe6001d90
	bx lr

	.global syscall_ed
syscall_ed:
	.long 0xe6001db0
	bx lr

	.global syscall_ee
syscall_ee:
	.long 0xe6001dd0
	bx lr

	.global syscall_ef
syscall_ef:
	.long 0xe6001df0
	bx lr

	.global syscall_f0
syscall_f0:
	.long 0xe6001e10
	bx lr

	.global syscall_f1
syscall_f1:
	.long 0xe6001e30
	bx lr

	.global syscall_f2
syscall_f2:
	.long 0xe6001e50
	bx lr

	.global syscall_f3
syscall_f3:
	.long 0xe6001e70
	bx lr

	.global syscall_f4
syscall_f4:
	.long 0xe6001e90
	bx lr

	.global syscall_f5
syscall_f5:
	.long 0xe6001eb0
	bx lr

	.global syscall_f6
syscall_f6:
	.long 0xe6001ed0
	bx lr

	.global syscall_f7
syscall_f7:
	.long 0xe6001ef0
	bx lr

	.global syscall_f8
syscall_f8:
	.long 0xe6001f10
	bx lr

	.global syscall_f9
syscall_f9:
	.long 0xe6001f30
	bx lr

	.global syscall_fa
syscall_fa:
	.long 0xe6001f50
	bx lr

	.global syscall_fb
syscall_fb:
	.long 0xe6001f70
	bx lr

	.global syscall_fc
syscall_fc:
	.long 0xe6001f90
	bx lr

	.global syscall_fd
syscall_fd:
	.long 0xe6001fb0
	bx lr

	.global syscall_fe
syscall_fe:
	.long 0xe6001fd0
	bx lr

	.global syscall_ff
syscall_ff:
	.long 0xe6001ff0
	bx lr

	.global svc_write
svc_write:
	mov r1, r0
	mov r0, #4
	svc #0xab
	bx lr

	.pool
	.end
