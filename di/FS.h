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
#ifndef __FS_STRUCT_H__
#define __FS_STRUCT_H__

#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "alloc.h"
#include "dip.h"

#define ISFS_MAX_PATH	0x40

typedef s32 (*isfscallback)(s32 result,void *usrdata);

typedef struct _fstats
{
	u32 Size;
	u32 Offset;
} fstats;

typedef struct 
{
	char filepath[0x40];
	union {
		char filepath_ren[0x40];
		struct {
			u32 owner_id;
			u16 group_id;
			char filepath[0x40];
			u8 ownerperm;
			u8 groupperm;
			u8 otherperm;
			u8 attributes;
			u8 pad0[2];
		} fsattr;
		struct {
			ioctlv vector[4];
			u32 no_entries;
		} fsreaddir;
		struct {
			ioctlv vector[4];
			u32 usage1;
			u8 pad0[28];
			u32 usage2;
		} fsusage;
		struct {
			u32	a;
			u32	b;
			u32	c;
			u32	d;
			u32	e;
			u32	f;
			u32	g;
		} fsstats;
	};

	isfscallback cb;
	void *usrdata;
	u32 functype;
	void *funcargv[8];
} isfs_cb;

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

#define ISFS_MAXPATH				13

#define ISFS_OPEN_READ				0x01
#define ISFS_OPEN_WRITE				0x02
#define ISFS_OPEN_RW				(ISFS_OPEN_READ | ISFS_OPEN_WRITE)

// FFS ioctl's
#define ISFS_IOCTL_FORMAT			1
#define ISFS_IOCTL_GETSTATS			2
#define ISFS_IOCTL_CREATEDIR		3
#define ISFS_IOCTL_READDIR			4
#define ISFS_IOCTL_SETATTR			5
#define ISFS_IOCTL_GETATTR			6
#define ISFS_IOCTL_DELETE			7
#define ISFS_IOCTL_RENAME			8
#define ISFS_IOCTL_CREATEFILE		9
#define ISFS_IOCTL_SETFILEVERCTRL	10
#define ISFS_IOCTL_GETFILESTATS		11
#define ISFS_IOCTL_GETUSAGE			12
#define ISFS_IOCTL_SHUTDOWN			13

#define ISFS_IS_USB					30

s32 ISFS_ReadDir( const char *filepath, char *name_list, u32 *num );
s32 ISFS_GetFileStats( s32 fd, fstats *status );
s32 ISFS_Delete( const char *filepath );
s32 ISFS_CreateFile( const char *FileName, u8 Attributes, u8 PermOwner, u8 PermGroup, u8 PermOther );
s32 ISFS_CreateDir( const char *FileName, u8 Attributes, u8 PermOwner, u8 PermGroup, u8 PermOther );
s32 ISFS_Rename( const char *FileSrc, const char *FileDst );

#endif
