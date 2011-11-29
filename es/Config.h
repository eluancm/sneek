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
#ifndef __CFG__
#define __CFG__

#include "global.h"
#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "common.h"
#include "FS.h"
#include "alloc.h"

typedef struct  {
	u8		MagicBytes[4];
	u16		ItemCount;
	u16		ItemOffset[];
} SysConfigHeader;

enum Area {
						AREA_JAP,
						AREA_USA,
						AREA_EUR,
						AREA_KOR
};

enum Game {
						JP,
						US,
						EU,
						KR,
};

enum Video {
						NTSC,
						PAL,
						MPAL,
};

enum Sound {			Monoral,
						Stereo,
						Surround
};

enum AspectRatio {		_4to3,
						_16to9
};

enum GeneralONOFF {		Off,
						On
};

enum SystemLanguage {	Japanse,
						English,
						German,
						French,
						Spanish,
						Italian,
						Dutch,
						ChineseSimple,
						ChineseTraditional,
						Korean
};

s32 Config_InitSYSCONF( void );
u32 Config_UpdateSysconf( void );

u32 SCSetByte( char *Name, u8 Value );

s32 Config_InitSetting( void );
s32 Config_SetSettingsValue( char *Name, u32 value );
s32 Config_SettingsFlush( void );
u32 Config_GetSettingsValue( char *Name );
char *Config_GetSettingsString( char *Name );

void Config_ChangeSystem( u64 TitleID, u16 TitleVersion );

#endif
