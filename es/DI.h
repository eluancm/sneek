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
#ifndef __DI_STRUCT_H__
#define __DI_STRUCT_H__

#include "global.h"
#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "common.h"
#include "alloc.h"

enum GameRegion 
{
	JAP=0,
	USA,
	EUR,
	KOR,
	ASN,
	LTN,
	UNK,
	ALL,
};

enum SNEEKConfig
{
	CONFIG_PATCH_FWRITE		= (1<<0),
	CONFIG_PATCH_MPVIDEO	= (1<<1),
	CONFIG_PATCH_VIDEO		= (1<<2),
	
	CONFIG_DUMP_ERROR_SKIP	= (1<<3),

	CONFIG_DEBUG_GAME		= (1<<4),
	CONFIG_DEBUG_GAME_WAIT	= (1<<5),
	
	CONFIG_SHOW_COVERS		= (1<<6),
	CONFIG_AUTO_UPDATE_LIST	= (1<<7),

	CONFIG_DUMP_MODE		= (1<<8),

	CONFIG_FAKE_CONSOLE_RG	= (1<<9),
};

enum HookTypes
{
	HOOK_TYPE_MASK		= (0xF<<28),
	
	HOOK_TYPE_VSYNC		= (1<<28),
	HOOK_TYPE_OSLEEP	= (2<<28),
	//HOOK_TYPE_AXNEXT	= (3<<28),

};

#define DVD_CONFIG_SIZE		0x10
#define DVD_GAMEINFO_SIZE	0x80
#define DVD_GAME_NAME_OFF	0x60

typedef struct
{
	u32		SlotID;
	u32		Region;
	u32		Gamecount;
	u32		Config;
	u8		GameInfo[][DVD_GAMEINFO_SIZE];
} DIConfig;

enum DIOpcodes
{
	DVD_IDENTIFY			= 0x12,
	DVD_READ_DISCID			= 0x70,
	DVD_LOW_READ			= 0x71,
	DVD_WAITFORCOVERCLOSE	= 0x79,
	DVD_READ_PHYSICAL		= 0x80,
	DVD_READ_COPYRIGHT		= 0x81,
	DVD_READ_DISCKEY		= 0x82,
	DVD_GETCOVER			= 0x88,
	DVD_RESET				= 0x8A,
	DVD_OPEN_PARTITION		= 0x8B,
	DVD_CLOSE_PARTITION		= 0x8C,
	DVD_READ_UNENCRYPTED	= 0x8D,
	DVD_ENABLE_VIDEO		= 0x8E,
	DVD_REPORTKEY			= 0xA4,
	DVD_LOW_SEEK			= 0xAB,
	//DVD_READ				= 0xD0,
	DVD_READ_CONFIG			= 0xD1,
	DVD_READ_BCA			= 0xDA,
	DVD_GET_ERROR			= 0xE0,
	DVD_SET_MOTOR			= 0xE3,

	DVD_SELECT_GAME			= 0x23,
	DVD_GET_GAMECOUNT		= 0x24,
	DVD_EJECT_DISC			= 0x27,
	DVD_INSERT_DISC			= 0x28,
	DVD_UPDATE_GAME_CACHE	= 0x2F,
	DVD_READ_GAMEINFO		= 0x30,
	DVD_WRITE_CONFIG		= 0x31,
	DVD_CONNECTED			= 0x32,

	DVD_OPEN				= 0x40,
	DVD_READ				= 0x41,
	DVD_WRITE				= 0x42,
	DVD_CLOSE				= 0x43,
	DVD_CREATEDIR			= 0x44,
};
enum DIError
{
	DI_SUCCESS				= 0x00,
	DI_ERROR				= 0x02,
	DI_FATAL				= 0x40,
};

#define		READSIZE	(32*1024)

#define		DIP_BASE	0x0D806000

#define		DIP_STATUS	(*(vu32*)(DIP_BASE+0x00))
#define		DIP_COVER	(*(vu32*)(DIP_BASE+0x04))
#define		DIP_CMD_0	(*(vu32*)(DIP_BASE+0x08))
#define		DIP_CMD_1	(*(vu32*)(DIP_BASE+0x0C))
#define		DIP_CMD_2	(*(vu32*)(DIP_BASE+0x10))
#define		DIP_DMA_ADR	(*(vu32*)(DIP_BASE+0x14))
#define		DIP_DMA_LEN	(*(vu32*)(DIP_BASE+0x18))
#define		DIP_CONTROL	(*(vu32*)(DIP_BASE+0x1C))
#define		DIP_IMM		(*(vu32*)(DIP_BASE+0x20))
#define		DIP_CONFIG	(*(vu32*)(DIP_BASE+0x24))

#define		DMA_READ		3
#define		IMM_READ		1

u32 DVDLowReadDiscID( void *data );
u32 DVDLowGetStatus( u32 slot );
u32 DVDLowRequestError( void );
s32 InitRegisters( void );
void DVDLowReset( void );
u32 DVDLowRead( void *data, u64 offset, u32 length );

s32 DVDCreateDir( char *FileName );
s32 DVDOpen( char *FileName );
s32 DVDWrite( s32 fd, void *ptr, u32 len );
s32 DVDRead( s32 fd, void *ptr, u32 len );
s32 DVDClose( s32 fd );

s32 DVDLowEnableVideo( u32 Mode );
s32 DVDGetGameCount( u32 *Mode );
s32 DVDSetRegion( u32 *Region );
s32 DVDGetRegion( u32 *Region );
s32 DVDEjectDisc( void );
s32 DVDInsertDisc( void );
s32 DVDUpdateGameCache( void );
s32 DVDReadGameInfo( u32 Offset, u32 Length, void *Data );
s32 DVDWriteDIConfig( void *DIConfig );
s32 DVDSelectGame( u32 SlotID );
s32 DVDConnected( void );

#endif