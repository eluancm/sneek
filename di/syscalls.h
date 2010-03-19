#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__	1

#include "global.h"
#include "ipc.h"

u32 ThreadCreate( u32 (*proc)(void* arg), u8 priority, u32* stack, u32 stacksize, void* arg, int autostart );
void ThreadJoin(void);
void ThreadCancel( u32 ThreadID, u32 when);

s32 GetProcessID(void);
s32 GetThreadID(void);

s32 ThreadContinue( s32 ThreadID );
s32 ThreadStop( u32 ThreadID );
void ThreadYield(void);
int ThreadGetPriority( int ThreadID );
void ThreadSetPriority( int ThreadID, int prio );

int MessageQueueCreate(void *ptr, int n);
void MessageQueueDestroy(int queue);
void MessageQueueSend(void);
int MessageQueueSendNow(int queue, struct ipcmessage *message, int flags);
int MessageQueueReceive( int QueueID, struct ipcmessage **message, int Flags );

void RegisterEventHandler(int device, int queue, int message);
void UnregisterEventHandler(void);

int  TimerCreate(int Time, int Dummy, int MessageQueue, int Message );
void TimerRestart( int TimerID );
void TimerStop( int TimerID );
void TimerDestroy( int TimerID );
int  TimerNow( int TimerID );

int HeapCreate(void *ptr, int Size);
void HeapDestroy(int HeapID);
void *HeapAlloc(int HeapID, int Size);
void *HeapAllocAligned(int HeapID, int Size, int align);
void HeapFree(int HeapID, void *ptr);

int IOS_Register(const char *device, int queue);
int IOS_Open(const char *device, int mode);
void IOS_Close(int fd);

int IOS_Read( int fd, void *ptr, u32 len );
int IOS_Write( int fd, void *ptr, u32 len );
int IOS_Seek( int fd, s32 where, s32 whence );
int IOS_Ioctl( int fd, s32 request, void *buffer_in, s32 bytes_in, void *buffer_io, s32 bytes_io);
int IOS_Ioctlv( int fd, int no, int bytes_in, int bytes_out, void *vec);

int IOS_OpenAsync( const char *filepath,u32 mode, int ipc_cb,void *usrdata );
int IOS_CloseAsync(int fd, int ipc_cb,void *usrdata );
int IOS_ReadAsync( int fd, void *ptr, u32 len, int ipc_cb,void *usrdata );
int IOS_WriteAsync( int fd, void *ptr, u32 len, int ipc_cb,void *usrdata );
int IOS_SeekAsync( int fd, s32 where, s32 whence, int ipc_cb,void *usrdata );
int IOS_IoctlAsync( int fd, s32 request, void *buffer_in, s32 bytes_in, void *buffer_io, s32 bytes_io, int ipc_cb,void *usrdata );
int IOS_IoctlvAsync( int fd, int no, int bytes_in, int bytes_out, void *vec, int ipc_cb,void *usrdata );


void MessageQueueAck( struct ipcmessage *message, int retval );

void SetUID(void);
void HMACGetQueueForPID(void);
void GetGID(void);
void syscall_2e(void);

void cc_ahbMemFlush( int device );
void _ahbMemFlush(int device );

void syscall_31(void);

void IRQ_Enable_18(void);
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
void sync_before_read(void *ptr, int len);
void sync_after_write(void *ptr, int len);

void syscall_41(void);
void syscall_42(void);
void syscall_43(void);

s32 DIResetAssert(void);
s32 DIResetDeAssert(void);
s32 DIResetCheck(void);

void syscall_47(void);
void syscall_48(void);
void syscall_49(void);
void syscall_4a(void);

#define DebugPrint(a) syscall_4b(a)
void syscall_4b(unsigned int);
void syscall_4c(void);
void syscall_4d(void);
void syscall_4e(void);

void *VirtualToPhysical(void *ptr);

unsigned int EnableVideo( unsigned int );
void syscall_51(void);
void EXICtrl( s32 Flag );
void syscall_53(void);
void syscall_54(void);
void syscall_55(void);
void syscall_56(void);
void syscall_57(void);
void syscall_58(void);
void syscall_59(void);
void syscall_5a(void);
void syscall_5b(void);
void syscall_5c(void);
void syscall_5d(void);
void syscall_5e(void);
void syscall_5f(void);
void syscall_60(void);
void syscall_61(void);
void syscall_62(void);
void GetKey( u32 KeyID, u8 *Key);
void syscall_64(void);
void syscall_65(void);
void syscall_66(void);

int AESEncrypt(void *, void *, void *, int , void *);

void syscall_68(void);
void syscall_69(void);
void syscall_6a(void);

int AESDecrypt(int KeyID, void *iv, void *in, int len, void *out);

void syscall_6c(void);
void syscall_6d(void);
void syscall_6e(void);
void syscall_6f(void);
void syscall_70(void);
void syscall_71(void);
void syscall_72(void);
void syscall_73(void);
void syscall_74(void);
void syscall_75(void);
void syscall_76(void);
void syscall_77(void);
void syscall_78(void);
void syscall_79(void);
void syscall_7a(void);
void syscall_7b(void);
void syscall_7c(void);
void syscall_7d(void);
void syscall_7e(void);
void syscall_7f(void);

void OSReport(const char *);
s32 os_ioctl(s32 fd, s32 request, void *buffer_in, s32 bytes_in, void *buffer_io, s32 bytes_io);


#endif
