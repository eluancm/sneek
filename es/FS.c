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
#include "FS.h"

extern int FFSHandle;

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

s32 ISFS_Init( void )
{
	FFSHandle = IOS_Open("/dev/fs", 0 );
	dbgprintf("ES:IOS_Open(\"/dev/fs\", 0 ):%d\n", FFSHandle );

	u8 *stats = heap_alloc( 0, 0x1C );

	s32 r = ISFS_GetStats( stats );
	dbgprintf("ES:ISFS_GetStats():%d\n", r );

	free(stats );

	u32 *num = (u32*)malloca( 4, 32 );
	char *path = malloca( 0x70, 0x40 );

	_sprintf( path, "/sys" );
	r = ISFS_ReadDir( path, NULL, num );
	if( r < 0 )
	{
		r = ISFS_CreateDir( path, 0, 3, 3, 3 );
		dbgprintf("ES:ISFS_CreateDir(\"%s\"):%d\n", path, r );
	} else
		dbgprintf("ES:ISFS_ReadDir(\"%s\"):%d ItemCount:%d\n", path, r, *num );

	_sprintf( path, "/ticket" );
	r = ISFS_ReadDir( path, NULL, num );
	if( r < 0 )
	{
		r = ISFS_CreateDir( path, 0, 3, 3, 3 );
		dbgprintf("ES:ISFS_CreateDir(\"%s\"):%d\n", path, r );
	} else
		dbgprintf("ES:ISFS_ReadDir(\"%s\"):%d ItemCount:%d\n", path, r, *num );

	_sprintf( path, "/shared1" );
	r = ISFS_ReadDir( path, NULL, num );
	if( r < 0 )
	{
		r = ISFS_CreateDir( path, 0, 3, 3, 3 );
		dbgprintf("ES:ISFS_CreateDir(\"%s\"):%d\n", path, r );
	} else
		dbgprintf("ES:ISFS_ReadDir(\"%s\"):%d ItemCount:%d\n", path, r, *num );

	_sprintf( path, "/shared2" );
	r = ISFS_ReadDir( path, NULL, num );
	if( r < 0 )
	{
		r = ISFS_CreateDir( path, 0, 3, 3, 3 );
		dbgprintf("ES:ISFS_CreateDir(\"%s\"):%d\n", path, r );
	} else
		dbgprintf("ES:ISFS_ReadDir(\"%s\"):%d ItemCount:%d\n", path, r, *num );

	_sprintf( path, "/tmp" );
	r = ISFS_ReadDir( path, NULL, num );
	if( r < 0 )
	{
		r = ISFS_CreateDir( path, 0, 3, 3, 3 );
		dbgprintf("ES:ISFS_CreateDir(\"%s\"):%d\n", path, r );
	} else
		dbgprintf("ES:ISFS_ReadDir(\"%s\"):%d ItemCount:%d\n", path, r, *num );

	_sprintf( path, "/import" );
	r = ISFS_ReadDir( path, NULL, num );
	if( r < 0 )
	{
		r = ISFS_CreateDir( path, 0, 3, 3, 3 );
		dbgprintf("ES:ISFS_CreateDir(\"%s\"):%d\n", path, r );
	} else
		dbgprintf("ES:ISFS_ReadDir(\"%s\"):%d ItemCount:%d\n", path, r, *num );

	_sprintf( path, "/meta" );
	r = ISFS_ReadDir( path, NULL, num );
	if( r < 0 )
	{
		r = ISFS_CreateDir( path, 0, 3, 3, 3 );
		dbgprintf("ES:ISFS_CreateDir(\"%s\"):%d\n", path, r );
	} else
		dbgprintf("ES:ISFS_ReadDir(\"%s\"):%d ItemCount:%d\n", path, r, *num );

	_sprintf( path, "/shared1/content.map" );
	r = ISFS_CreateFile( path, 0, 3, 3, 3 );
	if( r != -105 )
	{
		dbgprintf("ES:ISFS_CreateFile(\"%s\"):%d\n", path, r );
	}

	free(num );
	free(path );

	return 0;
}
s32 ISFS_CreateFile( const char *FileName, u8 Attributes, u8 PermOwner, u8 PermGroup, u8 PermOther )
{
	isfs_cb *param = (isfs_cb*)malloca( sizeof(isfs_cb), 32 );
	memset32( param, 0, sizeof(isfs_cb) );

	memcpy8( param->fsattr.filepath, FileName, strlen(FileName)+1 );
	
	param->fsattr.attributes = Attributes;
	param->fsattr.ownerperm = PermOwner;
	param->fsattr.groupperm = PermGroup;
	param->fsattr.otherperm = PermOther;

	s32 r = IOS_Ioctl( FFSHandle, ISFS_IOCTL_CREATEFILE, &param->fsattr, sizeof(param->fsattr), NULL, 0 );

	free( param );

	return r;
}
s32 ISFS_CreateDir( const char *FileName, u8 Attributes, u8 PermOwner, u8 PermGroup, u8 PermOther )
{
	isfs_cb *param = (isfs_cb*)malloca( sizeof(isfs_cb), 32 );
	memset32( param, 0, sizeof(isfs_cb) );

	memcpy8( param->fsattr.filepath, FileName, strlen(FileName)+1 );
	
	param->fsattr.attributes = Attributes;
	param->fsattr.ownerperm = PermOwner;
	param->fsattr.groupperm = PermGroup;
	param->fsattr.otherperm = PermOther;

	s32 r = IOS_Ioctl( FFSHandle, ISFS_IOCTL_CREATEDIR, &param->fsattr, sizeof(param->fsattr), NULL, 0 );

	free(param );

	return r;
}
s32 ISFS_GetStats( void *stats )
{
	return IOS_Ioctl( FFSHandle, ISFS_IOCTL_GETSTATS, NULL, 0, stats, 0x1C );
}
s32 ISFS_ReadDir( const char *filepath, char *name_list, u32 *num )
{
	if( name_list == NULL )
	{
		vector *v = (vector*)malloca( sizeof(vector)*2, 0x40 );

		v[0].data = (u32)filepath;
		v[0].len  = sizeof( char * );
		v[1].data = (u32)num;
		v[1].len  = sizeof( u32 * );

		s32 r = IOS_Ioctlv( FFSHandle, ISFS_IOCTL_READDIR, 1, 1, v );

		free(v );

		return r;
	} else {
		vector *v = (vector*)malloca( sizeof(vector)*2, 0x40 );

		v[0].data = (u32)filepath;
		v[0].len  = sizeof( char * );
		v[1].data = (u32)num;
		v[1].len  = sizeof( u32 * );

		v[2].data = (u32)name_list;
		v[2].len  = *num*13;
		v[3].data = (u32)num;
		v[3].len  = sizeof( u32 * );

		s32 r = IOS_Ioctlv( FFSHandle, ISFS_IOCTL_READDIR, 2, 2, v );

		free(v );

		return r;
	}
}
s32 ISFS_GetFileStats( s32 fd, fstats *status )
{
	return IOS_Ioctl( fd, ISFS_IOCTL_GETFILESTATS, NULL, 0, status, sizeof(fstats) );
}
s32 ISFS_Delete( const char *filepath )
{
	return IOS_Ioctl( FFSHandle, ISFS_IOCTL_DELETE, (void*)filepath, 0x40, NULL, 0 );
}

s32 ISFS_GetUsage( const char* filepath, u32* usage1, u32* usage2 )
{
	isfs_cb *param = (isfs_cb*)malloca( sizeof(isfs_cb), 32 );
	memset32( param, 0, sizeof(isfs_cb) );

	memcpy8( param->filepath, filepath, strlen(filepath)+1 );

	param->fsusage.vector[0].data = param->filepath;
	param->fsusage.vector[0].len = ISFS_MAXPATH;
	param->fsusage.vector[1].data = &param->fsusage.usage1;
	param->fsusage.vector[1].len = sizeof(u32);
	param->fsusage.vector[2].data = &param->fsusage.usage2;
	param->fsusage.vector[2].len = sizeof(u32);

	s32 r = IOS_Ioctlv( FFSHandle, ISFS_IOCTL_GETUSAGE, 1, 2, param->fsusage.vector );

	if( usage1 != NULL )
	{
		if( ((u32)(usage1)&3) == 0 )
		{
			*usage1 = param->fsusage.usage1;
		} else {
			dbgprintf("ES:Warning unaligned memory access:%p\n", usage1 );
		}
	}

	if( usage2 != NULL )
	{
		if( ((u32)(usage2)&3) == 0 )
		{
			*usage2 = param->fsusage.usage2;
		} else {
			dbgprintf("ES:Warning unaligned memory access:%p\n", usage2 );
		}
	}

	free( param );

	return r;
}
s32 ISFS_Rename(const char *filepathOld,const char *filepathNew )
{
	isfs_cb *param = (isfs_cb*)malloca( sizeof(isfs_cb), 32 );
	memset32( param, 0, sizeof(isfs_cb) );

	memcpy8( param->filepath, filepathOld, strlen(filepathOld)+1 );
	memcpy8( param->filepath_ren, filepathNew,  strlen(filepathNew)+1 );

	s32 r = IOS_Ioctl( FFSHandle, ISFS_IOCTL_RENAME, param->filepath, (ISFS_MAXPATH<<1), NULL, 0 );

	free( param );

	return r;
}

