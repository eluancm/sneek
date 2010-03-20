/*

SNEEK - SD-NAND/ES emulation kit for Nintendo Wii

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
};

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
	DVD_READ				= 0xD0,
	DVD_READ_CONFIG			= 0xD1,
	DVD_READ_BCA			= 0xDA,
	DVD_GET_ERROR			= 0xE0,
	DVD_SET_MOTOR			= 0xE3,

	DVD_SELECT_GAME			= 0x23,
	DVD_GET_GAMECOUNT		= 0x24,
	DVD_EJECT_DISC			= 0x27,
	DVD_INSERT_DISC			= 0x28,
	DVD_READ_GAMEINFO		= 0x30,
};
enum DIError
{
	DI_SUCCESS				= 0x00,
	DI_ERROR				= 0x02,
	DI_FATAL				= 0x40,
};

s32 DVDLowEnableVideo( u32 Mode );
s32 DVDGetGameCount( u32 *Mode );
s32 DVDSetRegion( u32 *Region );
s32 DVDGetRegion( u32 *Region );
s32 DVDEjectDisc( void );
s32 DVDInsertDisc( void );
s32 DVDReadGameInfo( u32 SlotID, u8 *GameInfo );
s32 DVDSelectGame( u32 SlotID );

#endif