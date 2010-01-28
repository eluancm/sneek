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

	s32 r = IOS_Ioctl( fd, IOCTL_DVDLOWENABLEVIDEO, m, 1, NULL, 0 );

	IOS_Close( fd );

	heap_free( 0, m );

	return r;
}