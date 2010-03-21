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
s32 DVDReadGameInfo( u32 SlotID, u8 *GameInfo )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	u32 *vec = malloca( sizeof(u32) * 2, 32 );
	vec[0] = SlotID;
	vec[1] = (u32)GameInfo;

	s32 r = IOS_Ioctl( fd, DVD_READ_GAMEINFO, vec, sizeof(u32) * 2, NULL, 0 );

	IOS_Close( fd );

	free( vec );

	return r;
}

s32 DVDSelectGame( u32 SlotID )
{
	s32 fd = IOS_Open("/dev/di", 0 );
	if( fd < 0 )
		return fd;

	u32 *vec = malloca( sizeof(u32) * 1, 32 );
	vec[0] = SlotID;

	s32 r = IOS_Ioctl( fd, DVD_SELECT_GAME, vec, sizeof(u32) * 1, NULL, 0 );

	IOS_Close( fd );

	free( vec );

	return r;
}
