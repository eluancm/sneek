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

#ifndef _FS_
#define _FS_

#include "global.h"
#include "ff.h"
#include "syscalls.h"
#include "ipc.h"
#include "string.h"
#include "utils.h"
#include "vsprintf.h"

#define MAX_FILE		20
#define FS_FD			0x99		// 153
#define B2_FD			0x98		// 152	
#define FL_FD			0x97		// 151
#define SD_FD			0x96		// 153

// error codes
#define FS_SUCCESS		0			// Success
#define FS_EACCES		-1			// Permission denied 
#define FS_EEXIST		-2			// File exists 
#define FS_EINVAL		-4			// Invalid argument, Invalid FD
#define FS_ENOENT		-6			// File not found 
#define FS_EBUSY		-8			// Resource busy 
#define FS_EIO			-12			// returned on ECC error 
#define FS_ENOMEM		-22			// Alloc failed during request 
#define FS_EFATAL		-101		// Fatal error 
#define FS_EACCESS		-102		// Permission denied 
#define FS_ECORRUPT		-103		// returned for "corrupted" NAND 
#define FS_EEXIST2		-105		// File exists 
#define FS_ENOENT2		-106		// File not found 
#define FS_ENFILE		-107		// Too many fds open 
#define FS_EFBIG		-108		// max block count reached? 
#define FS_ENFILE2 		-109		// Too many fds open 
#define FS_ENAMELEN		-110		// pathname is too long 
#define FS_EFDOPEN		-111		// FD is already open 
#define FS_EIO2			-114		// returned on ECC error 
#define FS_ENOTEMPTY 	-115		// Directory not empty 
#define FS_EDIRDEPTH	-116		// max directory depth exceeded 
#define FS_EBUSY2		-118		// Resource busy 
//#define FS_EFATAL		-119		// fatal error, not used by IOS as fatal ERROR

// FFS ioctl's
#define	IOCTL_INIT       0x01
#define	IOCTL_NANDSTATS  0x02
#define	IOCTL_CREATEDIR  0x03
#define	IOCTL_READDIR    0x04
#define	IOCTL_SETATTR    0x05
#define	IOCTL_GETATTR    0x06
#define	IOCTL_DELETE     0x07
#define	IOCTL_RENAME     0x08
#define	IOCTL_CREATEFILE 0x09
#define	IOCTL_GETSTATS   0x0B
#define	IOCTL_GETUSAGE   0x0C
#define	IOCTL_SHUTDOWN   0x0D

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

typedef struct
{
	u32 BlockSize;
	u32 FreeBlocks;
	u32 UsedBlocks;
	u32 unk3;
	u32 unk4;
	u32 Free_INodes;
	u32 unk5;
} NANDStat;

typedef struct {
	u32 file_length;
	u32 file_pos;
} FDStat;


void FFS_Ioctl(struct IPCMessage *msg);
void FFS_Ioctlv(struct IPCMessage *msg);
u32 FS_CheckHandle( s32 fd );
s32 FS_GetUsage( char *path, u32 *FileCount, u32 *TotalSize );
s32 FS_ReadDir( char *Path, u32 *FileCount, char *FileNames );
s32 FS_CreateFile( char *Path );
s32 FS_Delete( char *Path );
s32 FS_Close( s32 FileHandle );
s32 FS_Open( char *Path, u8 Mode );
s32 FS_Write( s32 FileHandle, u8 *Data, u32 Length );
s32 FS_Read( s32 FileHandle, u8 *Data, u32 Length );
s32 FS_Seek( s32 FileHandle, s32 Where, u32 Whence );
s32 FS_GetStats( s32 FileHandle, FDStat *Stats );
s32 FS_CreateDir( char *Path );
s32 FS_SetAttr( char *Path );
s32 FS_DeleteFile( char *Path );
s32 FS_Move( char *sPath, char *dPath );

#endif
