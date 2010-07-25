#ifndef _DIGLUE_
#define _DIGLUE_

#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "alloc.h"
#include "dip.h"
#include "ff.h"
#include "FS.h"

#define MAX_HANDLES		10

#define FS_ENOENT2		-106
#define ISFS_IS_USB		30

enum FSMODE
{
	SNEEK = 0,
	UNEEK = 1,
};

enum DVDOMODE
{
	DREAD	= 1,
	DWRITE	= 2,
	DCREATE	= 4,
};

enum DVDStatus
{
	DVD_SUCCESS	=	0,
	DVD_NO_FILE =	-106,
	DVD_FATAL	=	-101,
};

s32 ISFS_IsUSB( void );

void DVDInit( void );

s32 DVDOpen( char *Filename, u32 Mode );
void DVDClose( s32 handle );

u32 DVDGetSize( s32 handle );
s32 DVDDelete( char *Path );
s32 DVDCreateDir( char *Path );

u32 DVDRead( s32 handle, void *ptr, u32 len );
u32 DVDWrite( s32 handle, void *ptr, u32 len );
s32 DVDSeek( s32 handle, s32 where, u32 whence );

s32 DVDOpenDir( char *Path );
s32 DVDReadDir( void );
s32 DVDDirIsFile( void );
char *DVDDirGetEntryName( void );


#endif
