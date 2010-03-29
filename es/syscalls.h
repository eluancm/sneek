#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__	1

#include "global.h"

#define thread_create( proc, priority, stack, stacksize, arg, autostart ) syscall_00( proc, priority, stack, stacksize, arg, autostart )
u32 syscall_00( u32 (*proc)(void* arg), u8 priority, u32* stack, u32 stacksize, void* arg, bool autostart );

void syscall_01(void);

#define ThreadCancel( ThreadID, when ) syscall_02(ThreadID, when)
void syscall_02( u32 ThreadID, u32 when);

void syscall_03(void);
#define GetPID( void ) syscall_04( void )
u32 syscall_04( void );

#define thread_continue( ThreadID ) syscall_05( ThreadID )
s32 syscall_05( s32 ThreadID );

#define thread_stop( ThreadID ) syscall_06( ThreadID )
s32 syscall_06( u32 ThreadID );
void syscall_07(void);
void syscall_08(void);

#define thread_set_priority(a,b) syscall_09(a,b)
int syscall_09( int ThreadID, int prio);

#define mqueue_create(a, b) syscall_0a(a, b)
int syscall_0a(void *ptr, int n);

#define mqueue_destroy(a) syscall_0b(a)
void syscall_0b(int queue);

void syscall_0c(void);

#define mqueue_send_now(a, b, c) syscall_0d(a, b, c)
int syscall_0d(int queue, void *message, int flags);

#define mqueue_recv(a, b, c) syscall_0e(a, b, c)
int syscall_0e(int queue, void *message, int flags);

#define irq_register_handler(device, queue, message) syscall_0f(device, queue, message)
void syscall_0f(int device, int queue, int message);

void syscall_10(void);

int  TimerCreate(int Time, int Dummy, int MessageQueue, int Message );
void TimerRestart( int TimerID, int Dummy, int Time );
void TimerStop( int TimerID );
void TimerDestroy( int TimerID );
int  TimerNow( int TimerID );

#define heap_create(a, b) syscall_16(a, b)
int syscall_16(void *ptr, int len);

#define heap_destroy(a) syscall_17(a)
void syscall_17(int heapid);

#define heap_alloc(a, b) syscall_18(a, b)
void *syscall_18(int heap, int size);

#define heap_alloc_aligned(a, b, c) syscall_19(a, b, c)
void *syscall_19(int heap, int size, int align);

#define heap_free(a, b) syscall_1a(a, b)
void syscall_1a(int, void *);

#define device_register(a, b) syscall_1b(a, b)
int syscall_1b(const char *device, int queue);

s32 IOS_Open(const char *device, int mode);
void IOS_Close(int fd);
s32 IOS_Read(s32 fd,void *buffer,u32 length);
s32 IOS_Write( s32 fd, void *buffer, u32 length );
s32 IOS_Seek( u32 fd, u32 where, u32 whence );

s32 IOS_Ioctl(s32 fd, s32 request, void *buffer_in, s32 bytes_in, void *buffer_io, s32 bytes_io);
s32 IOS_Ioctlv(s32 fd, s32 request, s32 InCount, s32 OutCont, void *vec);

#define IOS_OpenAsync( filepath, mode, ipc_cb, usrdata ) syscall_23( filepath, mode, ipc_cb, usrdata )
s32 syscall_23( const char *filepath,u32 mode,ipccallback ipc_cb,void *usrdata );
void syscall_24(void);
void syscall_25(void);
void syscall_26(void);
void syscall_27(void);
void syscall_28(void);
void syscall_29(void);

#define mqueue_ack(a, b) syscall_2a(a, b)
void syscall_2a(void *message, int retval);

#define SetUID(PID, b) syscall_2b(PID, b)
u32 syscall_2b( u32 PID, u32 b);

void syscall_2c(void);

#define _cc_ahbMemFlush(a,b) syscall_2d(a,b)
u32 syscall_2d( u32 pid, u32 b);

void syscall_2e(void);

#define cc_ahbMemFlush(a) syscall_2f(a)
void syscall_2f( int device );

#define _ahbMemFlush(a) syscall_30(a)
void syscall_30(int device );

void syscall_31(void);
void IRQ18(void);
void syscall_33(void);

#define irq_enable(a) syscall_34(a)
int syscall_34(int device);

void syscall_35(void);
void syscall_36(void);
void syscall_37(void);
void syscall_38(void);
void syscall_39(void);
void syscall_3a(void);
void syscall_3b(void);
void syscall_3c(void);
void syscall_3d(void);
void syscall_3e(void);

#define sync_before_read(a, b) sync_before_read(a, b)
void sync_before_read(void *ptr, int len);

void sync_after_write(void *ptr, int len);

#define PPCBoot( Path ) syscall_41( Path )
s32 syscall_41( char *Path );

#define IOSBoot( Path, unk, Version ) syscall_42( Path, unk, Version )
s32 syscall_42( char *Path, u32 Flag, u32 Version );

#define IOSUnk( Offset, b ) syscall_43( Offset, b )
s32 syscall_43( u32 Offset, u32 b );

void DIResetAssert( void );
void DIResetDeAssert( void );
u32 DIResetCheck( void );

#define GetFlags( Flag1, Flag2 ) syscall_47( Flag1, Flag2 )
void syscall_47( u32 *Flag1, u16 *Flag2 );

void syscall_48(void);
void syscall_49(void);
void syscall_4a(void);

#define DebugPrint(a) syscall_4b(a)
void syscall_4b(unsigned int);

#define SetKernelVersion( Version ) syscall_4c( Version )
s32 syscall_4c( u32 Version );

#define GetKernelVersion( void ) syscall_4d( void )
s32 syscall_4d(void);

void syscall_4e( u32 value );	//Poke GPIO OUT

#define virt_to_phys(a) syscall_4f(a)
unsigned int syscall_4f(void *ptr);

#define EnableVideo(a) syscall_50(a)
u32 syscall_50( u32 );

void syscall_51(void);	//Get Some DIFLAG
void EXICtrl( u32 flag );
void syscall_53(void);

#define DoStuff(a) syscall_54(a)
void syscall_54( u32 a );
void syscall_55(void);
void syscall_56(void);
void syscall_57(void);
void syscall_58(void);

#define LoadPPC( TMD ) syscall_59( TMD )
s32 syscall_59( u8 *TMD);

#define LoadModule( Path ) syscall_5a( Path )
s32 syscall_5a( char *Path );


s32 CreateKey( u32 *Object, u32 ValueA, u32 ValueB );
void DestroyKey( u32 KeyID );
s32 syscall_5d(u32 a, u32 b, u32 c, u32 d, u32 e, void *f, void *g);
void syscall_5e(void);
s32 syscall_5f( u8 *unknown, u32 b, u32 c );
void syscall_60(void);
s32 syscall_61(u32 a, u32 b, u32 c);
void syscall_62(void);
void GetKey( u32 KeyID, u8 *Key );
void syscall_64(void);
void syscall_65(void);
void syscall_66(void);

s32 sha1( void *SHACarry, void* data, u32 len, u32 SHAMode, void *hash);

void syscall_68(void);
int aes_encrypt(int keyid, void *iv, void *in, int len, void *out);

void syscall_6a(int keyid, void *iv, void *in, int len, void *out);

#define aes_decrypt_( KeyID, iv, in, len, out) syscall_6b(KeyID, iv, in, len, out)
int syscall_6b(int keyid, void *iv, void *in, int len, void *out);

s32 syscall_6c( void *data, u32 b, u32 c, u32 d );
void syscall_6d(void);
void syscall_6e(void);
s32 syscall_6f( void *data, u32 ObjectA, u32 ObjectB);
void GetDeviceCert( void *data);
s32 syscall_71(u32 a, u32 b);
void syscall_72(void);
void syscall_73(void);
s32 syscall_74( u32 Key );
s32 syscall_75( u8 *hash, u32 HashLength, u32 Key, u8 *Signature );
s32 syscall_76( u32 Key, char *Issuer, u8 *Cert );
void syscall_77(void);
void syscall_78(void);
void syscall_79(void);
void syscall_7a(void);
void syscall_7b(void);
void syscall_7c(void);
void syscall_7d(void);
void syscall_7e(void);
void syscall_7f(void);
void syscall_80(void);
void syscall_81(void);
void syscall_82(void);
void syscall_83(void);
void syscall_84(void);
void syscall_85(void);
void syscall_86(void);
void syscall_87(void);
void syscall_88(void);
void syscall_89(void);
void syscall_8a(void);
void syscall_8b(void);
void syscall_8c(void);
void syscall_8d(void);
void syscall_8e(void);
void syscall_8f(void);
void syscall_90(void);
void syscall_91(void);
void syscall_92(void);
void syscall_93(void);
void syscall_94(void);
void syscall_95(void);
void syscall_96(void);
void syscall_97(void);
void syscall_98(void);
void syscall_99(void);
void syscall_9a(void);
void syscall_9b(void);
void syscall_9c(void);
void syscall_9d(void);
void syscall_9e(void);
void syscall_9f(void);
void syscall_a0(void);
void syscall_a1(void);
void syscall_a2(void);
void syscall_a3(void);
void syscall_a4(void);
void syscall_a5(void);
void syscall_a6(void);
void syscall_a7(void);
void syscall_a8(void);
void syscall_a9(void);
void syscall_aa(void);
void syscall_ab(void);
void syscall_ac(void);
void syscall_ad(void);
void syscall_ae(void);
void syscall_af(void);
void syscall_b0(void);
void syscall_b1(void);
void syscall_b2(void);
void syscall_b3(void);
void syscall_b4(void);
void syscall_b5(void);
void syscall_b6(void);
void syscall_b7(void);
void syscall_b8(void);
void syscall_b9(void);
void syscall_ba(void);
void syscall_bb(void);
void syscall_bc(void);
void syscall_bd(void);
void syscall_be(void);
void syscall_bf(void);
void syscall_c0(void);
void syscall_c1(void);
void syscall_c2(void);
void syscall_c3(void);
void syscall_c4(void);
void syscall_c5(void);
void syscall_c6(void);
void syscall_c7(void);
void syscall_c8(void);
void syscall_c9(void);
void syscall_ca(void);
void syscall_cb(void);
void syscall_cc(void);
void syscall_cd(void);
void syscall_ce(void);
void syscall_cf(void);
void syscall_d0(void);
void syscall_d1(void);
void syscall_d2(void);
void syscall_d3(void);
void syscall_d4(void);
void syscall_d5(void);
void syscall_d6(void);
void syscall_d7(void);
void syscall_d8(void);
void syscall_d9(void);
void syscall_da(void);
void syscall_db(void);
void syscall_dc(void);
void syscall_dd(void);
void syscall_de(void);
void syscall_df(void);
void syscall_e0(void);
void syscall_e1(void);
void syscall_e2(void);
void syscall_e3(void);
void syscall_e4(void);
void syscall_e5(void);
void syscall_e6(void);
void syscall_e7(void);
void syscall_e8(void);
void syscall_e9(void);
void syscall_ea(void);
void syscall_eb(void);
void syscall_ec(void);
void syscall_ed(void);
void syscall_ee(void);
void syscall_ef(void);
void syscall_f0(void);
void syscall_f1(void);
void syscall_f2(void);
void syscall_f3(void);
void syscall_f4(void);
void syscall_f5(void);
void syscall_f6(void);
void syscall_f7(void);
void syscall_f8(void);
void syscall_f9(void);
void syscall_fa(void);
void syscall_fb(void);
void syscall_fc(void);
void syscall_fd(void);
void syscall_fe(void);
void syscall_ff(void);

void svc_write(const char *);
s32 os_ioctl(s32 fd, s32 request, void *buffer_in, s32 bytes_in, void *buffer_io, s32 bytes_io);


#endif
