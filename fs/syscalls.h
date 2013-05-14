#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__	1

#include "global.h"

#define thread_create( proc, priority, stack, stacksize, arg, autostart ) syscall_00( proc, priority, stack, stacksize, arg, autostart )
u32 syscall_00( u32 (*proc)(void* arg), void* arg, u32* stack, u32 stacksize, u8 priority, u32 autostart );

void syscall_01(void);

#define ThreadCancel( ThreadID, when ) syscall_02(ThreadID, when)
void syscall_02( u32 ThreadID, u32 when);

void syscall_03(void);
void syscall_04(void);

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
void syscall_17(int HeapID);

#define heap_alloc(a, b) syscall_18(a, b)
void *syscall_18(int Heap, int size);

#define heap_alloc_aligned( HeapID, Size, Align) syscall_19(HeapID, Size, Align)
void *syscall_19(int Heap, int size, int align);

#define heap_free(a, b) syscall_1a(a, b)
void syscall_1a(int, void *);

#define device_register(a, b) syscall_1b(a, b)
int syscall_1b(const char *device, int queue);

#define IOS_Open(a, b) syscall_1c(a, b)
int syscall_1c(const char *device, int mode);

#define IOS_Close(a) syscall_1d(a)
void syscall_1d(int fd);

void syscall_1e(void);
void syscall_1f(void);
void syscall_20(void);
//void syscall_21(void);

int device_ioctl_async(int fd, u32 request, void *input_buffer, u32 input_buffer_len, void *output_buffer, u32 output_buffer_len, int queueid, void *message);

#define ios_ioctl(fd, request, buffer_in, bytes_in, buffer_io, bytes_io) syscall_21(fd, request, buffer_in, bytes_in, buffer_io, bytes_io)
#define os_ioctl(a, b, c, d, e, f) syscall_21(a, b, c, d, e, f)
s32 syscall_21(s32 fd, s32 request, void *buffer_in, s32 bytes_in, void *buffer_io, s32 bytes_io);

#define ios_ioctlv(a, b, c, d, e) syscall_22(a, b, c, d, e)
int syscall_22(int fd, int no, int bytes_in, int bytes_out, void *vec);

#define IOS_OpenAsync( filepath, mode, ipc_cb, usrdata ) syscall_23( filepath, mode, ipc_cb, usrdata )
s32 syscall_23( const char *filepath,u32 mode, u32 ipc_cb, void *usrdata );


#define mqueue_ack(a, b) syscall_2a(a, b)
void syscall_2a(void *message, int retval);


#define cc_ahbMemFlush(a) syscall_2f(a)
void syscall_2f( int device );

#define _ahbMemFlush(a) syscall_30(a)
void syscall_30(int device );

#define irq_enable(a) syscall_34(a)
int syscall_34(int device);

#define sync_before_read(a, b) sync_before_read(a, b)
void sync_before_read(void *ptr, int len);

#define sync_after_write(a, b) sync_after_write(a, b)
void sync_after_write(void *ptr, int len);


#define DebugPrint(a) syscall_4b(a)
void syscall_4b(unsigned int);

#define virt_to_phys(a) syscall_4f(a)
unsigned int syscall_4f(void *ptr);

#define EnableVideo(a) syscall_50(a)
unsigned int syscall_50( unsigned int );


s32 KeyCreate( u32 *Object, u32 ValueA, u32 ValueB );
void KeyDelete( u32 KeyID );
s32 KeyInitialize(u32 a, u32 b, u32 c, u32 d, u32 e, void *f, void *g);

#define aes_decrypt_( KeyID, iv, in, len, out) syscall_6b(KeyID, iv, in, len, out)
int syscall_6b(int keyid, void *iv, void *in, int len, void *out);


void svc_write(const char *);


#endif
