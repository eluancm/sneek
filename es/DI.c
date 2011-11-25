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
#include "DI.h"


s32 DVDLowEnableVideo( u32 Mode )
{
	if( Mode == 0 )
		return DI_SUCCESS;

	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	u8 *m = heap_alloc( 0, sizeof(u8) );
	*m = 0;

	s32 r = IOS_Ioctl( fd, DVD_ENABLE_VIDEO, m, 1, NULL, 0 );

	IOS_Close( fd );

	heap_free( 0, m );

	return r;
}
s32 DVDLowPrepareCoverRegister( u32 *Cover )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	s32 r = IOS_Ioctl( fd, 0x7A, NULL, 0, Cover, sizeof(u32) );

	IOS_Close( fd );

	return r;
}
s32 DVDGetGameCount( u32 *Count )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	s32 r = IOS_Ioctl( fd, DVD_GET_GAMECOUNT, NULL, 0, Count, sizeof(u32) );

	IOS_Close( fd );

	return r;
}
s32 DVDEjectDisc( void )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	s32 r = IOS_Ioctl( fd, DVD_EJECT_DISC, NULL, 0, NULL, 0 );

	IOS_Close( fd );

	return r;
}
s32 DVDInsertDisc( void )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	s32 r = IOS_Ioctl( fd, DVD_INSERT_DISC, NULL, 0, NULL, 0 );

	IOS_Close( fd );

	return r;
}
s32 DVDReadGameInfo( u32 Offset, u32 Length, void *Data )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	u32 *vec = (u32 *)malloca( sizeof(u32) * 3, 32 );
	vec[0] = Offset;
	vec[1] = Length;
	vec[2] = (u32)Data;

	s32 r = IOS_Ioctl( fd, DVD_READ_GAMEINFO, vec, sizeof(u32) * 3, NULL, 0 );

	IOS_Close( fd );

	free( vec );

	return r;
}
s32 DVDWriteDIConfig( void *DIConfig )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	u32 *vec = (u32 *)malloca( sizeof(u32) * 1, 32 );
	vec[0] = (u32)DIConfig;

	s32 r = IOS_Ioctl( fd, DVD_WRITE_CONFIG, vec, sizeof(u32) * 1, NULL, 0 );

	IOS_Close( fd );

	free( vec );

	return r;
}
s32 DVDSelectGame( u32 SlotID )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	u32 *vec = (u32 *)malloca( sizeof(u32) * 1, 32 );
	vec[0] = SlotID;

	s32 r = IOS_Ioctl( fd, DVD_SELECT_GAME, vec, sizeof(u32) * 1, NULL, 0 );

	IOS_Close( fd );

	free( vec );

	return r;
}
s32 DVDConnected( void )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	s32 r = IOS_Ioctl( fd, DVD_CONNECTED, NULL, 0, NULL, 0);
	
	IOS_Close( fd );

	return r;
}

s32 DVDOpen( char *FileName )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;
		
	vector *v = (vector*)malloca( sizeof(vector), 32 );
	char *lFileName = (char*)malloca( strlen(FileName), 32 );

	strcpy( lFileName, FileName );

	v[0].data = (u32)lFileName;
	v[0].len = strlen(lFileName);

	s32 r = IOS_Ioctlv( fd, DVD_OPEN, 1, 0, v );
	
	free( lFileName );
	free( v );

	IOS_Close( fd );

	return r;
}
s32 DVDCreateDir( char *FileName )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;
		
	vector *v = (vector*)malloca( sizeof(vector), 32 );
	char *lFileName = (char*)malloca( strlen(FileName), 32 );

	strcpy( lFileName, FileName );

	v[0].data = (u32)lFileName;
	v[0].len = strlen(lFileName);

	s32 r = IOS_Ioctlv( fd, DVD_CREATEDIR, 1, 0, v );
	
	free( lFileName );
	free( v );

	IOS_Close( fd );

	return r;
}
s32 DVDRead( s32 fd, void *ptr, u32 len )
{
	s32 DVDHandle = IOS_Open("/dev/di", 0 );
	if( DVDHandle < 0 )
		return DVDHandle;

	vector *v = (vector*)malloca( sizeof(vector)*2, 32 );
	
	v[0].data = fd;
	v[0].len = sizeof(u32);
	v[1].data = (u32)ptr;
	v[1].len = len;

	s32 r = IOS_Ioctlv( DVDHandle, DVD_READ, 2, 0, v );

	free( v );

	IOS_Close( DVDHandle );

	return r;
}
s32 DVDWrite( s32 fd, void *ptr, u32 len )
{
	s32 DVDHandle = IOS_Open("/dev/di", 0 );
	if( DVDHandle < 0 )
		return DVDHandle;

	vector *v = (vector*)malloca( sizeof(vector)*2, 32 );
	
	v[0].data = fd;
	v[0].len = sizeof(u32);
	v[1].data = (u32)ptr;
	v[1].len = len;

	s32 r = IOS_Ioctlv( DVDHandle, DVD_WRITE, 2, 0, v );

	free( v );

	IOS_Close( DVDHandle );

	return r;
}
s32 DVDClose( s32 fd )
{
	s32 DVDHandle = IOS_Open("/dev/di", 0 );
	if( DVDHandle < 0 )
		return DVDHandle;

	vector *v = (vector*)malloca( sizeof(vector), 32 );

	v[0].data = fd;
	v[0].len = sizeof(u32);

	s32 r = IOS_Ioctlv( DVDHandle, DVD_CLOSE, 1, 0, v );

	free( v );

	IOS_Close( DVDHandle );

	return r;
}
u32 DVDLowRead( void *data, u64 offset, u32 length )
{
	DIP_STATUS  = 0x2A|4|0x10;
	DIP_CMD_0	= 0xA8000000;
	DIP_CMD_1	= (u32)(offset>>2);
	DIP_CMD_2	= length;
	DIP_DMA_LEN	= length;
	DIP_DMA_ADR	= (u32)data;
	DIP_IMM		= 0;

	sync_before_read( data, length );

	DIP_CONTROL = DMA_READ;

	while (1)
	{
		if( DIP_STATUS & 0x4 )
			return 1;
		if( !DIP_DMA_LEN )
			return 0;
	}

	return 0;
}
u32 DVDLowReadDiscID( void *data )
{
	DIP_STATUS  = 0x2A|4|0x10;
	DIP_CMD_0	= 0xA8000040;
	DIP_CMD_1	= 0;
	DIP_CMD_2	= 0x20;
	DIP_DMA_LEN	= 0x20;
	DIP_DMA_ADR	= (u32)data;
	DIP_IMM		= 0;

	sync_before_read( data, 0x20 );

	DIP_CONTROL = DMA_READ;

	while (1)
	{
		if( DIP_STATUS & (1<<2) )
			return 1;
		if( DIP_STATUS & (1<<4) )
			return 0;
	}

	return 0;
}
u32 DVDLowRequestError( void )
{
	DIP_STATUS	= 0x2E;
	DIP_CMD_0	= 0xE0000000;
	DIP_IMM		= 0;
	DIP_CONTROL	= IMM_READ;

	while( DIP_CONTROL & 1 );

	return DIP_IMM;
}

void DVDLowReset( void )
{		
	*(vu32*)0xd800194 &= 0xFFFDFBFF;
	*(vu32*)0xd800194 |= 0x00020400;
}
