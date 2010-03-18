#include "dip.h"

s32 DIReadDiscID( void )
{
	s32 r = DIResetCheck();
	dbgprintf("DIResetCheck():%d\n", r );

	if( (r << 0x18) == 0 )
	{
		r = DI_CMD0;

		r|= 0xA8000000;
		DI_CMD0 = r;

		r = DI_CMD0;

		r|= 0x00000040;
		DI_CMD0 = r;
		DI_CMD1 = 0;
		DI_CMD2 = 0x20;
		DI_DMA_LEN = 0x20;
		DI_DMA_ADR = 0;

		DI_CR = DMA_READ;

		while (1)
		{
			if ( DI_STATUS & 0x4)
				return 1;
			if (!DI_DMA_LEN)
				return 0;
		}
	
	} else {
		dbgprintf("(%s) Read Disk ID called while in reset", "handleDiCommand" );
	}

	return 0;
}
void SetStatus_2( void )
{
	s32 r = DI_STATUS;
	r &= ~0x54;
	r |=  0x02;
	DI_STATUS = r;
}
void SetStatus( void )
{
	s32 r = DI_STATUS;
	r &= ~0x54;
	r |=  0x08;
	DI_STATUS = r;
}
void DoStatusMagic( void )
{
	u32 r = DI_STATUS;
	r &= ~0x54;
	r |=  0x10;
	DI_STATUS = r;
}
void CoverDisableIRQ( void )
{
	s32 r = DI_COVER;
	r &= ~4;
	r &= ~2;
	DI_COVER = r;
}
void CoverDisableIRQSetFlag( void )
{
	s32 r = DI_COVER;
	r &= ~4;
	r |= 2;
	DI_COVER = r;
}
s32 InitRegisters( void )
{
	DoStatusMagic();

	u32 r = DI_STATUS;
	r &= ~0x54;
	r |=  0x04;
	DI_STATUS = r;

	SetStatus();

	r = DI_STATUS;
	dbgprintf("DI_STATUS:%08X\n", r );

	if( ((r << 28) >> 31) == 0 )
	{
		u32 r2,r3,r4=1;

		do {
			udelay( 1000 );
			SetStatus();
			r3 = DI_STATUS;
			r2 = r4;
			r3 = r3 >> 3;
			r2&= ~r3;
		} while(r2);
	}

	SetStatus_2();
	CoverDisableIRQ();
	r = DI_STATUS;
	dbgprintf("DI_STATUS:%08X\n", r );

	if( (r << 29) >> 31 )
	{
		dbgprintf("(%s) **** Error interrupt cannot be cleared after reset ***", "initializeDriveRegisters" );
	}

	if( ((r>>4)&1) )
	{
		dbgprintf("(%s) ***** Trans complete interrupt cannot be cleared after reset ****", "initializeDriveRegisters" );
	}

	IRQ_Enable_18();

	return 0;
	
}