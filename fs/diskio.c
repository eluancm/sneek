/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	glue layer for FatFs

Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2008, 2009	Haxx Enterprises <bushing@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "diskio.h"
#include "string.h"
#include "sdmmc.h"
#include "syscalls.h"

DSTATUS disk_initialize( BYTE drv )
{
	if (sdmmc_check_card() == SDMMC_NO_CARD)
		return STA_NOINIT;

	sdmmc_ack_card();
	return disk_status(drv);
}

DSTATUS disk_status( BYTE drv )
{
	(void)drv;
	if (sdmmc_check_card() == SDMMC_INSERTED)
		return 0;
	else
		return STA_NODISK;
}

DRESULT disk_read( BYTE drv, BYTE *buff, DWORD sector, BYTE count )
{
	if( (u32)buff & 0xF0000000 )
	{
		u8 *buffer = (u8*)heap_alloc_aligned( 0, count*512, 0x40 );

		sdmmc_read( sector, count, buffer ) ;

		_ahbMemFlush( 9 );
		memcpy( buff, buffer, count*512 );

		heap_free( 0, buffer );
	} else {
		sdmmc_read( sector, count, buff ) ;
	}

	return RES_OK;
}
// Write Sector(s)
DRESULT disk_write( BYTE drv, const BYTE *buff, DWORD sector, BYTE count )
{
	u8 *buffer = (u8*)heap_alloc_aligned( 0, count*512, 0x40 );

	memcpy( buffer, buff, count*512 );

	_ahbMemFlush( 0xA );

	if( sdmmc_write( sector, count, buffer ) < 0 )
	{
		;//FS_Fatal( "disk_write()", __LINE__, __FILE__, sector, "Failed to read disc\n" );
	}

	heap_free( 0, buffer );

	return RES_OK;
}
