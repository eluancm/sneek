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

#ifndef _SDI_
#define _SDI_

//#define SDI

#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "gecko.h"
#include "alloc.h"
#include "vsprintf.h"
#include "sdmmcreg.h"
#include "sdhcreg.h"

typedef struct {
	u32 command;
	u32 type;			// 11b ABORT, 10b RESUME, 01b SUSPEND, 00b NORMAL
	u32 rep;			// 11b 48bytes Check Busy, 10b 48bytes, 01b 136bytes, 00b NONE
	u32 arg;			// RW
	u32 blocks;
	u32 bsize;
	u32 addr;
	u32 isDMA;
	u32 pad0;
} SDCommand;

void SD_Ioctl( struct ipcmessage *msg );
void SD_Ioctlv( struct ipcmessage *msg );

#endif
