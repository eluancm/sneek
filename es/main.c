/*

SNEEK - SD-NAND/ES emulation kit for Nintendo Wii

Copyright (C) 2009-2011  crediar

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
#include "common.h"
#include "sdhcreg.h"
#include "sdmmcreg.h"
#include "alloc.h"
#include "font.h"
#include "DI.h"
#include "SDI.h"
#include "SMenu.h"
#include "ES.h"

int verbose = 0;
u32 base_offset=0;
void *queuespace=NULL;
int queueid = 0;
int heapid=0;
int FFSHandle=0;
u32 FSUSB=0;

#undef DEBUG

extern u16 TitleVersion;
extern u32 *KeyID;
extern u8 *CNTMap;
extern u32 *HCR;
extern u32 *SDStatus;

int _main( int argc, char *argv[] )
{
	s32 ret=0;
	struct ipcmessage *message=NULL;
	u8 MessageHeap[0x10];
	u32 MessageQueue=0xFFFFFFFF;

	thread_set_priority( 0, 0x79 );	// do not remove this, this waits for FS to be ready!
	thread_set_priority( 0, 0x50 );
	thread_set_priority( 0, 0x79 );

#ifdef DEBUG
	dbgprintf("$IOSVersion: ES: %s %s 64M DEBUG$\n", __DATE__, __TIME__ );
#else
	dbgprintf("$IOSVersion: ES: %s %s 64M Release$\n", __DATE__, __TIME__ );
#endif

	MessageQueue = ES_Init( MessageHeap );
	
	s32 Timer = TimerCreate( 0, 0, MessageQueue, 0xDEADDEAD );
	
//SD Card 
	SDStatus = (u32*)malloca( sizeof(u32), 0x40 );
	*SDStatus = 0x00000002;

	HCR = (u32*)malloca( sizeof(u32)*0x30, 0x40 );
	memset32( HCR, 0, sizeof(u32)*0x30 );

	if( ISFS_IsUSB() == FS_ENOENT2 )
	{
		dbgprintf("ES:Found FS-SD\n");
		FSUSB = 0;

		ret = device_register("/dev/sdio", MessageQueue );
#ifdef DEBUG
		dbgprintf("ES:DeviceRegister(\"/dev/sdio\"):%d QueueID:%d\n", ret, queueid );
#endif
	} else {
		dbgprintf("ES:Found FS-USB\n");
		FSUSB = 1;
	}
	
	ret = 0;
	u32 MenuType = 0;
	u32 StartTimer = 1;

	if( LoadFont( "/sneek/font.bin" ) )	// without a font no point in displaying the menu
	{
		TimerRestart( Timer, 0, 1000000 );
	}
	
	if( GetTitleID() == 0x0000000100000002LL )
	{
		MenuType = 1;
		
	} else if ( (GetTitleID() >> 32) ==  0x00010001LL ) {
		//MenuType = 2;
		*SDStatus = 0x00000002;
	}

	LoadAndRebuildChannelCache();
		
	thread_set_priority( 0, 0x0A );

	dbgprintf("ES:looping!\n");
		
	while (1)
	{
		ret = mqueue_recv( MessageQueue, (void *)&message, 0);
		if( ret != 0 )
		{
			;//dbgprintf("ES:mqueue_recv(%d) FAILED:%d\n", MessageQueue, ret);
			return 0;
		}
	
		if( (u32)message == 0xDEADDEAD )
		{
			TimerStop( Timer );

			if( StartTimer )
			{
				StartTimer = 0;
				if( MenuType == 1 )
				{
					if( !SMenuFindOffsets( (void*)0x01330000, 0x003D0000 ) )
					{
						;//dbgprintf("ES:Failed to find all menu patches!\n");
						continue;
					}
				} else if( MenuType == 2 ) {
					if( !SMenuFindOffsets( (void*)0x00000000, 0x01200000 ) )
					{
						;//dbgprintf("ES:Failed to find all menu patches!\n");
						continue;
					}
				} else {
					continue;
				}
			}
			
			SMenuAddFramebuffer();
			if( MenuType == 1 )
			{
				SMenuDraw();
				SMenuReadPad();
			}
#ifdef CHEATMENU
			else if( MenuType == 2 ) {

				SCheatDraw();
				SCheatReadPad();
			}
#endif

			TimerRestart( Timer, 0, 2500 );
			continue;
		}

		//dbgprintf("ES:Command:%02X\n", message->command );
		//dbgprintf("ES:mqueue_recv(%d):%d cmd:%d ioctlv:\"%X\"\n", queueid, ret, message->command, message->ioctlv.command );
		
		switch( message->command )
		{
			case IOS_OPEN:
			{
				//dbgprintf("ES:mqueue_recv(%d):%d cmd:%d device:\"%s\":%d\n", queueid, ret, message->command, message->open.device, message->open.mode );
				// Is it our device?
				if( memcmp( message->open.device, "/dev/es", 8 ) == 0 )
				{
					ret = ES_FD;
				} else if( strncmp( message->open.device, "/dev/sdio/slot", 14 ) == 0 ) {
					ret = SD_FD;
				} else  {
					ret = FS_ENOENT;
				}
				
				mqueue_ack( (void *)message, ret );
				
			} break;
			
			case IOS_CLOSE:
			{
#ifdef DEBUG
				dbgprintf("ES:IOS_Close(%d)\n", message->fd );
#endif
				if( message->fd == ES_FD || message->fd == SD_FD )
				{
					mqueue_ack( (void *)message, ES_SUCCESS );
				} else 
					mqueue_ack( (void *)message, FS_EINVAL );

			} break;

			case IOS_READ:
			case IOS_WRITE:
			case IOS_SEEK:
			{
				mqueue_ack( (void *)message, FS_EINVAL );
			} break;
			case IOS_IOCTL:
			{
				if( message->fd == SD_FD ) 
					SD_Ioctl( message );
				else
					mqueue_ack( (void *)message, FS_EINVAL );

			} break;

			case IOS_IOCTLV:
			{
				if( message->fd == ES_FD )
				{
					ES_IoctlvN( message );
				} else if( message->fd == SD_FD ) {
					SD_Ioctlv( message );
				} else {
					mqueue_ack( (void *)message, FS_EINVAL );
				}
			} break;
			
			default:
				mqueue_ack( (void *)message, FS_EINVAL );
			break;
		}
	}

	return 0;
}

