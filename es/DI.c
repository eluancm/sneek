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

	v[0].data = (u32)FileName;
	v[0].len = strlen(FileName);

	s32 r = IOS_Ioctlv( fd, DVD_OPEN, 1, 0, v );

	free( v );

	IOS_Close( fd );

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
	s32 r = DIResetCheck();
	//dbgprintf("DIResetCheck():%d\n", r );

	if( (r << 0x18) == 0 )
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
	
	} else {
		dbgprintf("(%s) Read Disk ID called while in reset", "handleDiCommand" );
	}

	return 0;
}
u32 DVDLowReadDiscID( void *data )
{
	s32 r = DIResetCheck();
	//dbgprintf("DIResetCheck():%d\n", r );

	if( (r << 0x18) == 0 )
	{
		sync_before_read( data, 0x20 );

		u32 val = DIP_CMD_0;
			val|= 0xA8000000;
		DIP_CMD_0 = val;

		val = DIP_CMD_0;
		val|= 0x40;
		DIP_CMD_0 = val;

		DIP_CMD_1	= 0;
		DIP_CMD_2	= 0x20;
		DIP_DMA_LEN = 0x20;
		DIP_DMA_ADR = (u32)data;

		val = DIP_STATUS;
		val|= (1<<3)|(1<<4);
		val|= (1<<1)|(1<<2);
		DIP_STATUS = val;

		DIP_CONTROL = DMA_READ;

		while (1)
		{
			if( DIP_STATUS & (1<<2) )
				return 1;
			if( DIP_STATUS & (1<<4) )
				return 0;
		}
	
	} else {
		dbgprintf("(%s) Read Disk ID called while in reset", "handleDiCommand" );
	}

	return 0;
}
u32 DVDLowSeek( u64 offset )
{
	DIP_COVER = DIP_COVER;

	DIP_STATUS	= 0x3A;
	DIP_CMD_0	= 0xAB000000;
	DIP_CMD_1	= offset>>2;
	DIP_IMM		= 0xdead;

	DIP_CONTROL	= IMM_READ;

	while( DIP_CONTROL & 1 );

	return DIP_IMM;	
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

void DVDEnableCoverIRQ( void )
{
	s32 r = DIP_STATUS;
	r &= ~0x54;
	r |=  0x02;
	DIP_STATUS = r;
}
void DVDEnableTransferIRQ( void )
{
	s32 r = DIP_STATUS;
	r &= ~0x54;
	r |=  0x08;
	DIP_STATUS = r;
}
void DVDClearTransferIRQ( void )
{
	u32 r = DIP_STATUS;
	r &= ~0x54;
	r |=  0x10;
	DIP_STATUS = r;
}
void CoverDisableIRQ( void )
{
	s32 r = DIP_COVER;
	r &= ~4;
	r &= ~2;
	DIP_COVER = r;
}
void CoverDisableIRQSetFlag( void )
{
	s32 r = DIP_COVER;
	r &= ~4;
	r |= 2;
	DIP_COVER = r;
}
s32 InitRegisters( void )
{
	DVDClearTransferIRQ();

	u32 r = DIP_STATUS;
	r &= ~0x54;
	r |=  0x04;
	DIP_STATUS = r;

	DVDEnableTransferIRQ();

	r = DIP_STATUS;
	dbgprintf("DIP_STATUS:%08X\n", r );

	if( ((r << 28) >> 31) == 0 )
	{
		u32 r2,r3,r4=1;

		do {
			udelay( 1000 );
			DVDEnableTransferIRQ();
			r3 = DIP_STATUS;
			r2 = r4;
			r3 = r3 >> 3;
			r2&= ~r3;
		} while(r2);
	}

	DVDEnableCoverIRQ();
	CoverDisableIRQ();
	r = DIP_STATUS;
	dbgprintf("DIP_STATUS:%08X\n", r );

	if( (r << 29) >> 31 )
	{
		dbgprintf("(%s) **** Error interrupt cannot be cleared after reset ***", "initializeDriveRegisters" );
	}

	if( ((r>>4)&1) )
	{
		dbgprintf("(%s) ***** Trans complete interrupt cannot be cleared after reset ****", "initializeDriveRegisters" );
	}

	IRQ18();

	return 0;
	
}
void DVDLowReset( void )
{
	if( (DIResetCheck() << 24) == 0 )
	{
		DIResetAssert();

	} else {

		if( (DIP_COVER << 30) & (1<<31) )
		{
			CoverDisableIRQ();
			DIResetDeAssert();
			InitRegisters();
			CoverDisableIRQSetFlag();
			IRQ18();

		} else {

			DIResetDeAssert();
			InitRegisters();
			IRQ18();
		}

	}
}
void DVDInit( void )
{
	if( (DIResetCheck()<<0x18) == 0 )
		IRQ18();

	u32 r = DIResetCheck(); 
	dbgprintf("DIResetCheck():%d\n", r );

	if( (r << 24) != 0 )
	{
		r = DIP_COVER;
		dbgprintf("DIP_COVER():%d\n", r );
		if( r&2 )
		{
			CoverDisableIRQ();
			DIResetDeAssert();
			InitRegisters();
		} else {
			DIResetDeAssert();
			InitRegisters();
			CoverDisableIRQ();
		}
		IRQ18();
	} else {
		
		s32 r = DIP_COVER;
		dbgprintf("DIP_COVER():%d\n", r );
		if( (r << 30) >> 31 )
		{
			CoverDisableIRQ();

			DIResetAssert();

			udelay( 12 );

			DIResetDeAssert();

			InitRegisters();

			CoverDisableIRQSetFlag();

			IRQ18();

		} else {

			DIResetAssert();
			udelay( 12 );
			DIResetDeAssert();

			InitRegisters();

			IRQ18();
		}
	}
}

