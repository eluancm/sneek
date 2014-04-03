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
#include "FS.h"

extern int FSHandle;

s32 ISFS_CreateFile( const char *FileName, u8 Attributes, u8 PermOwner, u8 PermGroup, u8 PermOther )
{
	isfs_cb *param = (isfs_cb*)malloca( sizeof(isfs_cb), 32 );
	memset32( param, 0, sizeof(isfs_cb) );

	memcpy( param->fsattr.filepath, (void*)FileName, strlen(FileName)+1 );
	
	param->fsattr.attributes = Attributes;
	param->fsattr.ownerperm = PermOwner;
	param->fsattr.groupperm = PermGroup;
	param->fsattr.otherperm = PermOther;

	s32 r = IOS_Ioctl( FSHandle, ISFS_IOCTL_CREATEFILE, &param->fsattr, sizeof(param->fsattr), NULL, 0 );

	free( param );

	return r;
}
s32 ISFS_CreateDir( const char *FileName, u8 Attributes, u8 PermOwner, u8 PermGroup, u8 PermOther )
{
	isfs_cb *param = (isfs_cb*)malloca( sizeof(isfs_cb), 32 );
	memset32( param, 0, sizeof(isfs_cb) );

	memcpy( param->fsattr.filepath, (void*)FileName, strlen(FileName)+1 );
	
	param->fsattr.attributes = Attributes;
	param->fsattr.ownerperm = PermOwner;
	param->fsattr.groupperm = PermGroup;
	param->fsattr.otherperm = PermOther;

	s32 r = IOS_Ioctl( FSHandle, ISFS_IOCTL_CREATEDIR, &param->fsattr, sizeof(param->fsattr), NULL, 0 );

	free(param );

	return r;
}
s32 ISFS_ReadDir( const char *filepath, char *name_list, u32 *num )
{
	if( name_list == NULL )
	{
		vector *v = (vector*)halloca( sizeof(vector)*2, 0x40 );

		*v[0].data = (u32)filepath;
		v[0].len  = sizeof( char * );
		*v[1].data = (u32)num;
		v[1].len  = sizeof( u32 * );

		s32 r = IOS_Ioctlv( FSHandle, ISFS_IOCTL_READDIR, 1, 1, v );

		hfree(v);
		return r;

	} else {

		vector *v = (vector*)halloca( sizeof(vector)*4, 0x40 );

		*v[0].data = (u32)filepath;
		v[0].len  = sizeof( char * );
		*v[1].data = (u32)num;
		v[1].len  = sizeof( u32 * );

		*v[2].data = (u32)name_list;
		v[2].len  = *num*64;
		*v[3].data = (u32)num;
		v[3].len  = sizeof( u32 * );

		s32 r = IOS_Ioctlv( FSHandle, ISFS_IOCTL_READDIR, 2, 2, v );

		hfree(v);
		return r;
	}
}
s32 ISFS_GetFileStats( s32 fd, fstats *status )
{
	return IOS_Ioctl( fd, ISFS_IOCTL_GETFILESTATS, NULL, 0, status, sizeof(fstats) );
}
s32 ISFS_Delete( const char *filepath )
{
	return IOS_Ioctl( FSHandle, ISFS_IOCTL_DELETE, (void*)filepath, 0x40, NULL, 0 );
}
s32 ISFS_Rename( const char *FileSrc, const char *FileDst )
{
	u8  *data = ( u8*)malloca( 0x80, 32 );

	memcpy( data,	   (void*)FileSrc, 0x40 );
	memcpy( data+0x40, (void*)FileDst, 0x40 );

	s32 r = IOS_Ioctl( FSHandle, ISFS_IOCTL_RENAME, data, 0x80, NULL, 0 );

	free( data );

	return r;
}
