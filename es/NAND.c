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
#include "NAND.h"

extern u8  *CNTMap;

u8 *NANDLoadFile( char *path, u32 *Size )
{
	s32 fd = IOS_Open( path, 1 );
	if( fd < 0 )
	{
		dbgprintf("ES:NANDLoadFile->IOS_Open(\"%s\", 1 ):%d\n", path, fd );
		*Size = fd;
		return NULL;
	}

	//dbgprintf("ES:NANDLoadFile->IOS_Open(\"%s\", 1 ):%d\n", path, fd );

	fstats *status = (fstats*)heap_alloc_aligned( 0, sizeof(fstats), 0x40 );

	s32 r = ISFS_GetFileStats( fd, status );
	//dbgprintf("ES:NANDLoadFile->ISFS_GetFileStats(\"%s\", 1 ):%d\n", path, r );
	if( r < 0 )
	{
		dbgprintf("ES:NANDLoadFile->ISFS_GetFileStats(%d, %p ):%d\n", fd, status, r );
		heap_free( 0, status );
		*Size = r;
		return NULL;
	}

	*Size = status->Size;
	//dbgprintf("ES:NANDLoadFile->Size:%d\n", *Size );

	u8 *data = (u8*)heap_alloc_aligned( 0, status->Size, 0x40 );
	if( data == NULL )
	{
		dbgprintf("ES:NANDLoadFile(\"%s\")->Failed to alloc %d bytes!\n", path, status->Size );
		heap_free( 0, status );
		return NULL;
	}

	r = IOS_Read( fd, data, status->Size );
	//dbgprintf("ES:NANDLoadFile->IOS_Read():%d\n", r );
	if( r < 0 )
	{
		dbgprintf("ES:NANDLoadFile->IOS_Read():%d\n", r );
		heap_free( 0, status );
		*Size = r;
		return NULL;
	}

	heap_free( 0, status );
	IOS_Close( fd );

	return data;
}
s32 NANDWriteFileSafe( char *pathdst, void *data, u32 size )
{
	//Create file in tmp folder and move it to destination
	char *path = (char*)heap_alloc_aligned( 0, 0x40, 32 );

	_sprintf( path, "/tmp/file.tmp" );

	s32 r = ISFS_CreateFile( path, 0, 3, 3, 3 );
	if( r == FS_EEXIST2 )
	{
		ISFS_Delete( path );
		r = ISFS_CreateFile( path, 0, 3, 3, 3 );
		if( r < 0 )
		{
			heap_free( 0, path );
			return r;
		}
	} else {
		if( r < 0 )
		{
			heap_free( 0, path );
			return r;
		}
	}

	s32 fd = IOS_Open( path, 2 );
	if( fd < 0 )
	{
		heap_free( 0, path );
		return r;
	}

	r = IOS_Write( fd, data, size );
	if( r < 0 || r != size )
	{
		IOS_Close( fd );
		heap_free( 0, path );
		return r;
	}

	IOS_Close( fd );

	r = ISFS_Rename( path, pathdst );
	if( r < 0 )
	{
		heap_free( 0, path );
		return r;
	}

	heap_free( 0, path );
	return r;
}
