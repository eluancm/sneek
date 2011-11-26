#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__	1

#include "global.h"

u32 thread_create( u32 (*proc)(void* arg), u8 priority, u32* stack, u32 stacksize, void* arg, bool autostart );

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

#define mqueue_ack(a, b) syscall_2a(a, b)
void syscall_2a(void *message, int retval);

#define SetUID(PID, b) syscall_2b(PID, b)
u32 syscall_2b( u32 PID, u32 b);

void syscall_2c(void);

#define SetGID(a,b) syscall_2d(a,b)
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

#define DoStuff(a) syscall_54(a)
void syscall_54( u32 a );

#define LoadPPC( TMD ) syscall_59( TMD )
s32 syscall_59( u8 *TMD );

#define LoadModule( Path ) syscall_5a( Path )
s32 syscall_5a( char *Path );


s32 CreateKey( u32 *Object, u32 ValueA, u32 ValueB );
void DestroyKey( u32 KeyID );
s32 syscall_5d(u32 a, u32 b, u32 c, u32 d, u32 e, void *f, void *g);
s32 syscall_5f( u8 *unknown, u32 b, u32 c );
s32 syscall_61(u32 a, u32 b, u32 c);
void GetKey( u32 KeyID, u8 *Key );


s32 sha1( void *SHACarry, void* data, u32 len, u32 SHAMode, void *hash);

void syscall_68(void);
int aes_encrypt(int keyid, void *iv, void *in, int len, void *out);

void syscall_6a(int keyid, void *iv, void *in, int len, void *out);

#define aes_decrypt_( KeyID, iv, in, len, out) syscall_6b(KeyID, iv, in, len, out)
int syscall_6b(int keyid, void *iv, void *in, int len, void *out);

s32 syscall_6c( void *data, u32 b, u32 c, u32 d );

s32 syscall_6f( void *data, u32 ObjectA, u32 ObjectB);
void GetDeviceCert( void *data);
s32 syscall_71(u32 a, u32 b);
s32 syscall_74( u32 Key );
s32 syscall_75( u8 *hash, u32 HashLength, u32 Key, u8 *Signature );
s32 syscall_76( u32 Key, char *Issuer, u8 *Cert );


void svc_write(const char *);
s32 os_ioctl(s32 fd, s32 request, void *buffer_in, s32 bytes_in, void *buffer_io, s32 bytes_io);


#endif
