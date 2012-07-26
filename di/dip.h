/*

SNEEK - SD-NAND/ES + DI emulation kit for Nintendo Wii

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
#ifndef _DIP_
#define _DIP_

#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "gecko.h"
#include "alloc.h"
#include "vsprintf.h"
#include "DIGlue.h"
#include "Config.h"
#include "utils.h"

#define DI_SUCCESS	1
#define DI_ERROR	2
#define DI_FATAL	64

enum disctypes
{
	DISC_REV	= 0,
	DISC_DOL	= 1,
	DISC_INV	= 2,
};

enum opcodes
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
	DVD_REPORTKEY			= 0xA4,
	DVD_LOW_SEEK			= 0xAB,
	//DVD_READ				= 0xD0,
	DVD_READ_CONFIG			= 0xD1,
	DVD_READ_BCA			= 0xDA,
	DVD_GET_ERROR			= 0xE0,
	DVD_SET_MOTOR			= 0xE3,
	DVD_SET_AUDIO_BUFFER	= 0xE4,
	
	DVD_SELECT_GAME			= 0x23,
	DVD_GET_GAMECOUNT		= 0x24,
	DVD_EJECT_DISC			= 0x27,
	DVD_INSERT_DISC			= 0x28,
	DVD_UPDATE_GAME_CACHE	= 0x2F,
	DVD_READ_GAMEINFO		= 0x30,
	DVD_WRITE_CONFIG		= 0x31,
	DVD_CONNECTED			= 0x32, //Check if the harddrive is connected yet

	DVD_OPEN				= 0x40,
	DVD_READ				= 0x41,
	DVD_WRITE				= 0x42,
	DVD_CLOSE				= 0x43,
	DVD_CREATEDIR			= 0x44,

};

enum GameRegion 
{
	JAP=0,
	USA,
	EUR,
	KOR,
	ASN,
	LTN,
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

	CONFIG_FST_REBUILD_TEMP	= (1<<10),
	CONFIG_FST_REBUILD_PERMA= (1<<11),
	
	CONFIG_DML_CHEATS		= (1<<12),
	CONFIG_DML_DEBUGGER		= (1<<13),
	CONFIG_DML_DEBUGWAIT	= (1<<14),
	CONFIG_DML_NMM			= (1<<16),
	CONFIG_DML_ACTIVITY_LED	= (1<<17),
	CONFIG_DML_PADHOOK		= (1<<18),
	CONFIG_DML_BOOT_DISC	= (1<<19),
	CONFIG_DML_WIDESCREEN	= (1<<20),
	CONFIG_DML_PROG_PATCH	= (1<<27),
	
	CONFIG_DML_VID_AUTO		= (1<<21),
	CONFIG_DML_VID_FORCE	= (1<<22),
	CONFIG_DML_VID_NONE		= (1<<23),

	CONFIG_DML_VID_MASK		= CONFIG_DML_VID_AUTO|CONFIG_DML_VID_FORCE|CONFIG_DML_VID_NONE,

	CONFIG_DML_VID_FORCE_PAL50	= (1<<24),
	CONFIG_DML_VID_FORCE_PAL60	= (1<<25),
	CONFIG_DML_VID_FORCE_NTSC	= (1<<26),

	CONFIG_DML_VID_FORCE_MASK	= CONFIG_DML_VID_FORCE_PAL50|CONFIG_DML_VID_FORCE_PAL60|CONFIG_DML_VID_FORCE_NTSC,
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

typedef struct
{
	union
	{
		struct
		{
			u32 Type		:8;
			u32 NameOffset	:24;
		};
		u32 TypeName;
	};
	union
	{
		struct		// File Entry
		{
			u32 FileOffset;
			u32 FileLength;
		};
		struct		// Dir Entry
		{
			u32 ParentOffset;
			u32 NextOffset;
		};
		u32 entry[2];
	};
} FEntry;

typedef struct
{
	u32 Offset;
	u32 Size;
	s32 File;
} FileCache;


typedef struct
{
	u32 Length;
	u32 Loads;
	u32 Stores;
	u32 FCalls;
	u32 Branch;
	u32 Moves;
	char *Name;
} FuncPattern;

#define FILECACHE_MAX	5

typedef struct
{
	u8 *data;
	u32 len;
} vector;

typedef struct
{
	u32 TMDSize;
	u32 TMDOffset;
	u32	CertChainSize;
	u32 CertChainOffset;
	u32 H3TableOffset;
	u32	DataOffset;
	u32 DataSize;
} PartitionInfo;

u8 HardDriveConnected;//holds status of USB harddrive

int DIP_Ioctl( struct ipcmessage * );
int DIP_Ioctlv( struct ipcmessage * );
s32 DVDUpdateCache( u32 ForceUpdate );
s32 DVDSelectGame( int SlotID );
s32 DVDLowReadFiles( u32 Offset, u32 Length, void *ptr );
s32 DVDLowReadUnencrypted( u32 Offset, u32 Length, void *ptr );
s32 DVDLowReadDiscIDFiles( u32 Offset, u32 Length, void *ptr );
void DIP_Fatal( char *name, u32 line, char *file, s32 error, char *msg );

#endif