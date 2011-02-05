/*

SNEEK - SD-NAND/ES + DI emulation kit for Nintendo Wii

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
#include "diskio.h"

DSTATUS disk_initialize (BYTE drv)
{
	udelay( 50000 );

	tiny_ehci_init();

	while( ehci_discover() == -ENODEV )
		udelay( 4000 );

	s32 r = USBStorage_Init();
	
	u32 s_size;
	u32 s_cnt = USBStorage_Get_Capacity(&s_size);
	
	//dbgprintf("DIP: Drive size: %dMB SectorSize:%d\n", s_cnt / 1024 * s_size / 1024, s_size);

	return r;
}

DSTATUS disk_status (BYTE drv)
{
	(void)drv;
	return 0;
}

DRESULT disk_read (BYTE drv, BYTE *buff, DWORD sector, BYTE count)
{
	u32 *buffer = malloca( count*512, 0x40 );
	
	USBStorage_Read_Sectors( sector, count, buffer );
	memcpy( buff, buffer, count*512 );
	free( buffer );

	return RES_OK;
}

DRESULT disk_write (BYTE drv, const BYTE *buff, DWORD sector, BYTE count)
{
	u32 *buffer = malloca( count*512, 0x40 );
	memcpy( buffer, buff, count*512 );

	USBStorage_Write_Sectors( sector, count, buffer );
	free( buffer );

	return RES_OK;
}
