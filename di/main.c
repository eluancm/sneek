/*

SNEEK - SD-NAND/ES + DI emulation kit for Nintendo Wii

Copyright (C) 2009-2010  crediar

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "ehci_types.h"
#include "ehci.h"
#include "gecko.h"
#include "alloc.h"
#include "ff.h"
#include "dip.h"

extern u32 Region;
extern char *RegionStr;

void udelay(int us)
{
	u8 heap[0x10];
	struct ipcmessage *Message;
	int mqueue = -1;
	int TimeID = -1;

	mqueue = MessageQueueCreate(heap, 1);
	if(mqueue < 0)
		goto out;
	TimeID = TimerCreate(us, 0, mqueue, 0xbabababa);
	if(TimeID < 0)
		goto out;
	MessageQueueReceive(mqueue, &Message, 0);
	
out:
	if(TimeID > 0)
		TimerDestroy(TimeID);
	if(mqueue > 0)
		MessageQueueDestroy(mqueue);
}

s32 RegisterDevices( void *QueueSpace )
{
	int QueueID = MessageQueueCreate( QueueSpace, 8);

	s32 ret = IOS_Register("/dev/di", QueueID );
#ifdef DEBUG
	dbgprintf("DIP:DeviceRegister(\"/dev/di\"):%d\n", ret );
#endif
	if( ret < 0 )
		return ret;

	return QueueID;
}

char __aeabi_unwind_cpp_pr0[0];
void _main(void)
{
	FATFS fatfs;
	struct ipcmessage *IPCMessage=NULL;

	ThreadSetPriority( 0, 0xF4 );

#ifdef DEBUG
	dbgprintf("$IOSVersion: DIP: %s %s 64M DEBUG$\n", __DATE__, __TIME__ );
#else
	dbgprintf("$IOSVersion: DIP: %s %s 64M Release$\n", __DATE__, __TIME__ );
#endif

	if( GetThreadID() != 3 || GetProcessID() != 7 )
	{
		dbgprintf("PID:%d TID:%d\n", GetProcessID(), GetThreadID() );
		DIP_Fatal("main()", __LINE__, __FILE__, 0, "PID must be 7 and TID must be 3!");
	}

	HeapInit();

	void *QueueSpace = halloc( 0x20 );
	int QueueID = RegisterDevices( QueueSpace );
	if( QueueID < 0 )
	{
		ThreadCancel( 0, 0x77 );
	}
	
	s32 ret = EnableVideo(1);
	dbgprintf("DIP:EnableVideo(1):%d\n", ret );

	IRQ_Enable_18();
	s32 fres = f_mount(0, &fatfs );
	dbgprintf("DIP:f_mount():%d\n", fres);

	if( fres != FR_OK )
	{
		DIP_Fatal("main()", __LINE__, __FILE__, 0, "Could not find any USB device!");
		ThreadCancel( 0, 0x77 );
	}

	FIL fi;
	if( f_open( &fi, "/sneek/slot.bin", FA_READ ) == FR_OK )
	{
		u32 slot,read;

		f_read( &fi, &slot, sizeof(u32), &read );
		f_close( &fi );

		ret = DVDSelectGame( slot );
		dbgprintf("DIP:DVDSelectGame(%d):%d\n", slot, ret );

	} else {
		ret = DVDSelectGame( 0 );
		dbgprintf("DIP:DVDSelectGame():%d\n", ret );
	}

	if( f_open( &fi, "/sneek/region.bin", FA_READ ) == FR_OK )
	{
		u32 read;

		f_read( &fi, &Region, sizeof(u32), &read );
		f_close( &fi );

		if( Region > LTN )
			Region = JAP;

		dbgprintf("DIP:Region set to:%d\n", Region );

	} else {
		Region=JAP;
		dbgprintf("DIP:No region.bin found, set region to %d\n", Region );
	}

	while (1)
	{
		ret = MessageQueueReceive( QueueID, &IPCMessage, 0);
		if( ret != 0 )
		{
			dbgprintf("DIP: mqueue_recv(%d) FAILED:%d\n", QueueID, ret );
			continue;
		}

		switch( IPCMessage->command )
		{
			case IOS_OPEN:
			{
				if( strncmp("/dev/di", IPCMessage->open.device, 8 ) == 0 )
				{
					//dbgprintf("DIP:IOS_Open(\"%s\"):%d\n", IPCMessage->open.device, 24 );
					MessageQueueAck( IPCMessage, 24 );
				} else {
					//dbgprintf("DIP:IOS_Open(\"%s\"):%d\n", IPCMessage->open.device, -6 );
					MessageQueueAck( IPCMessage, -6 );
				}
				
			} break;

			case IOS_CLOSE:
			{
				//dbgprintf("DIP:IOS_Close(%d)\n", IPCMessage->fd );
				MessageQueueAck( IPCMessage, 0 );
			} break;

			case IOS_IOCTL:
				MessageQueueAck( IPCMessage, DIP_Ioctl( IPCMessage ) );
			break;

			case IOS_IOCTLV:
				MessageQueueAck( IPCMessage, DIP_Ioctlv( IPCMessage ) );
				break;

			case IOS_READ:
			case IOS_WRITE:
			case IOS_SEEK:
			default:
				dbgprintf("DIP: unimplemented/invalid msg: %08x\n", IPCMessage->command );
				MessageQueueAck( IPCMessage, -4);
				while(1);
		}
	}
}
