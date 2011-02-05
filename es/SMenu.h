
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

#ifndef _SMENU_
#define _SMENU_

#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "gecko.h"
#include "alloc.h"
#include "vsprintf.h"
#include "GCPad.h"
#include "WPad.h"
#include "DI.h"
#include "font.h"
#include "image.h"
#include "bmp.h"
#include "NAND.h"
#include "ES.h"

#define MAX_HITS			64
#define MAX_FB				3

#define MENU_POS_X			20
#define MENU_POS_Y			54

#define VI_INTERLACE		0
#define VI_NON_INTERLACE	1
#define VI_PROGRESSIVE		2

#define VI_NTSC				0
#define VI_PAL				1
#define VI_MPAL				2
#define VI_DEBUG			3
#define VI_DEBUG_PAL		4
#define VI_EUR60			5

void SMenuInit( u64 TitleID, u16 TitleVersion );
u32 SMenuFindOffsets( void *ptr, u32 SearchSize );
void SMenuAddFramebuffer( void );
void SMenuDraw( void );
void SMenuReadPad( void );

void SCheatDraw( void );
void SCheatReadPad( void );

void LoadAndRebuildChannelCache();

typedef struct{
	u64 titleID;
	u8 name[41];
} __attribute__((packed)) ChannelInfo;

typedef struct{
	u32 numChannels;
	ChannelInfo channels[];
} __attribute__((packed)) ChannelCache;

#endif
