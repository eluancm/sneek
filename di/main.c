/*

SNEEK - SD-NAND/ES + DI emulation kit for Nintendo Wii

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
#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "ehci_types.h"
#include "ehci.h"
#include "gecko.h"
#include "dip.h"
#include "alloc.h"

#define FAT

#ifdef FAT

#include "ff.h"

FATFS fatfs;
static FIL f;
static u8 GamePath[64];

#else

#define	SLOTSIZE	0x8C1200
#define CRED

#ifdef CRED
#define SEKTORPAD	63
#else
#define	SEKTORPAD	0
#endif

static u32 SlotID=0;

#endif
static u32 *KeyID ALIGNED(32);
void *queuespace = NULL;
int queueid = 0;
int heapid=0;
int tiny_ehci_init(void);
int ehc_loop(void);

typedef struct
{
	u8 *data;
	u32 len;
} vector;
typedef struct
{
	u32 TMDSize;
	u32 TMDOffset;
	u32	CertChainSize;
	u32 CertChainOffset;
	u32 H3TableOffset;
	u32	DataOffset;
	u32 DataSize;
} PartitionInfo;

int verbose = 0;
u32 Partition = 0;
u32 Motor = 0;
u32 Disc = 0;
u64 PartitionOffset=0;
u64 PartitionDataOffset=0;
u32 PartitionTableOffset=0;

void DIP_Fatal( char *name, u32 line, char *file, s32 error, char *msg )
{
	dbgprintf("************FATAL ERROR************\n");
	dbgprintf("Function :%s\n", name );
	dbgprintf("line     :%d\n", line );
	dbgprintf("file     :%s\n", file );
	dbgprintf("error    :%d\n", error );
	dbgprintf("%s\n", msg );
	dbgprintf("************FATAL ERROR************\n");

	while(1);
}
void udelay(int us)
{
	u8 heap[0x10];
	u32 msg;
	s32 mqueue = -1;
	s32 timer = -1;

	mqueue = mqueue_create(heap, 1);
	if(mqueue < 0)
		goto out;
	timer = timer_create(us, 0, mqueue, 0xbabababa);
	if(timer < 0)
		goto out;
	mqueue_recv(mqueue, &msg, 0);
	
out:
	if(timer > 0)
		timer_destroy(timer);
	if(mqueue > 0)
		mqueue_destroy(mqueue);
}

#define ALIGN_FORWARD(x,align) \
	((typeof(x))((((u32)(x)) + (align) - 1) & (~(align-1))))

#define ALIGN_BACKWARD(x,align) \
	((typeof(x))(((u32)(x)) & (~(align-1))))

static char ascii(char s)
{
  if(s < 0x20) return '.';
  if(s > 0x7E) return '.';
  return s;
}

void hexdump(void *d, int len) {
  u8 *data;
  int i, off;
  data = (u8*)d;
  for (off=0; off<len; off += 16) {
    dbgprintf("%08x  ",off);
    for(i=0; i<16; i++)
      if((i+off)>=len) dbgprintf("   ");
      else dbgprintf("%02x ",data[off+i]);

    dbgprintf(" ");
    for(i=0; i<16; i++)
      if((i+off)>=len) dbgprintf(" ");
      else dbgprintf("%c",ascii(data[off+i]));
    dbgprintf("\n");
  }
}


#ifndef FAT

u8 USBReadBuffer[0x200] ALIGNED(512);
static s32 ESHandle ALIGNED(32);
u32 USB_Read( u64 Offset, unsigned int Length, void *buffer )
{
	//dbgprintf("USB_Read( %X, %d, %p)\n", (u32)(Offset>>2), Length, buffer );

	u32 StartSector = Offset>>9;
	u32 AOffset		= Offset&0x1FF;
	s32	ret			= 0;
	u32 Size		= Length;
	u32 ReadSize	= 0x200;
	u32	BufferOff	= 0;

	if( AOffset )
	{
		ReadSize = 0x200-AOffset;
		if( ReadSize > Size )
			ReadSize = Size;

		//dbgprintf("ReadSize:%d AOffset:%d Size:%d\n", ReadSize, AOffset, Size );

		sync_before_read( USBReadBuffer, sizeof(USBReadBuffer) );
		ret = USBStorage_Read_Sectors( SEKTORPAD+SLOTSIZE*SlotID+StartSector, 1, USBReadBuffer );
		//dbgprintf("DIP:USBStorage_Read_Sectors( 0x%x, %d, 0x%p ):%d\n", StartSector, 1, USBReadBuffer, ret );

		sync_before_read( USBReadBuffer, sizeof(USBReadBuffer) );
		memcpy( buffer, USBReadBuffer+AOffset, ReadSize );

		Size -= ReadSize;
		StartSector+=1;
		BufferOff+=ReadSize;
	}

	if( Size>>9 )
	{
		ReadSize = (Size>>9)<<9;

		//dbgprintf("ReadSize:%d AOffset:%d Size:%d\n", ReadSize, AOffset, Size );

		sync_before_read( buffer+BufferOff, ReadSize );
		ret = USBStorage_Read_Sectors( SEKTORPAD+SLOTSIZE*SlotID+StartSector, (Size>>9), buffer+BufferOff );
		//dbgprintf("DIP:USBStorage_Read_Sectors( 0x%x, %d, 0x%p ):%d\n", StartSector, (Size>>9), buffer+BufferOff, ret );

		sync_before_read( buffer+BufferOff, ReadSize );

		StartSector+=(Size>>9);
		Size -= ReadSize;
		BufferOff+=ReadSize;
	}

	if( Size&0x1FF )
	{
		//dbgprintf("ReadSize:%d Size:%d\n", Size, Size );

		sync_before_read( USBReadBuffer, sizeof(USBReadBuffer) );
		ret = USBStorage_Read_Sectors( SEKTORPAD+SLOTSIZE*SlotID+StartSector, 1, USBReadBuffer );
		//dbgprintf("DIP:USBStorage_Read_Sectors( 0x%x, %d, 0x%p ):%d\n", StartSector, 1, USBReadBuffer, ret );

		sync_before_read( USBReadBuffer, sizeof(USBReadBuffer) );
		memcpy( buffer+BufferOff, USBReadBuffer, Size );
	}

	return DI_SUCCESS;
}
u32 USB_EncryptedRead( u64 Offset, unsigned int Length, void *buffer )
{
	//dbgprintf("DIP:USB_EncryptedRead( %X, %d, %p)\n", (u32)(Offset>>2), Length, buffer );

	Offset <<=2;

	u32 StartSector  = (u32)((u64)(((u64)Offset/(u64)0x7C00*(u64)0x8000)+PartitionDataOffset)>>9);

	u32 AOffset		= Offset%0x7C00;
	s32	ret			= 0;
	u32 Size		= (Length+31)&(~31);
	u32 ReadSize	= 0x7C00;
	u32	BufferOff	= 0;
	s32	r			= 0;

	u8 *enc		= heap_alloc_aligned( heapid, 0x8000, 64 );
	u8 *data	= heap_alloc_aligned( heapid, 0x7C00, 64 );

	ReadSize = 0x7C00-AOffset;
	if( ReadSize > Size )
		ReadSize = Size;

	while( Size )
	{
		//dbgprintf("DIP:StartSector:0x%x AOffset:0x%x ReadSize:0x%x Size:0x%x\n", StartSector, AOffset, ReadSize, Size );

		//sync_before_read( enc, 0x8000 );
		ret = USBStorage_Read_Sectors( SEKTORPAD+SLOTSIZE*SlotID+StartSector, 64, enc );
		//dbgprintf("DIP:USBStorage_Read_Sectors( 0x%x, %d, 0x%p ):%d\n", StartSector, 64, enc, ret );

		//sync_after_write( enc, 0x8000 );
		//cc_ahbMemFlush(1);

		//memcpy( iv, enc+0x3D0, 16 );

		//sync_after_write( iv, 16 );
		//cc_ahbMemFlush(1);

		//r = aes_decrypt_( 11, iv, enc, 0x3E0, hashes );
		//if(r<0)
		//{
		//	dbgprintf("DIP:aes_decrypt_( %d, %p, %p, %08X, %p ):%d\n", 11, iv, enc+0x400, AOffset+ReadSize, data, r );
		//	DIP_Fatal( "USB_EncryptedRead", __LINE__, __FILE__, r, "aes_decrypt failed");
		//}

		if( AOffset || (((u32)buffer+BufferOff)&0x3F) )
		{
			r = aes_decrypt_( *KeyID, enc+0x3D0, enc+0x400, 0x7c00, data );
		//	dbgprintf("DIP:aes_decrypt_( %d, %p, %p, %08X, %p ):%d\n", KeyID, enc+0x3D0, enc+0x400, 0x7c00, data, r );

			if( r < 0 )
			{
				dbgprintf("DIP:aes_decrypt_( %d, %p, %p, %08X, %p ):%d\n", *KeyID, enc+0x3D0, enc+0x400, 0x7c00, data, r );
				DIP_Fatal( "USB_EncryptedRead", __LINE__, __FILE__, r, "aes_decrypt failed");
			}

			//sync_after_write( data, AOffset+ReadSize );
			memcpy( buffer+BufferOff, data+AOffset, ReadSize );

		} else {
			r = aes_decrypt_( *KeyID, enc+0x3D0, enc+0x400, ReadSize, buffer+BufferOff );
		//	dbgprintf("DIP:aes_decrypt_( %d, %p, %p, %08X, %p ):%d\n", KeyID, enc+0x3D0, enc+0x400, ReadSize, buffer+BufferOff, r );
			if( r < 0 )
			{
				dbgprintf("DIP:aes_decrypt_( %d, %p, %p, %08X, %p ):%d\n", *KeyID, enc+0x3D0, enc+0x400, ReadSize, buffer+BufferOff, r );
				DIP_Fatal( "USB_EncryptedRead", __LINE__, __FILE__, r, "aes_decrypt failed");
			}
		}

		//sync_after_write( buffer+BufferOff, ReadSize );
		//cc_ahbMemFlush(4|2|1);

		Size		-= ReadSize;
		StartSector	+= 64;
		BufferOff	+= ReadSize;
		ReadSize	= 0x7C00;
		AOffset		= 0;

		if( Size < ReadSize )
			ReadSize = Size;
	}

	free( enc );
	free( data );

	return DI_SUCCESS;
}
#endif

static u32 error ALIGNED(32);
static u32 PartitionSize ALIGNED(32);
static u32 DIStatus ALIGNED(32);
static u32 DICover ALIGNED(32);
void DIP_Ioctl( struct ipcmessage *msg )
{
	u8  *bufin  = (u8*)msg->ioctl.buffer_in;
	u32 lenin   = msg->ioctl.length_in;
	u8  *bufout = (u8*)msg->ioctl.buffer_io;
	u32 lenout  = msg->ioctl.length_io;
	s32 ret = DI_FATAL;
#ifdef FAT
	u32 read =0 ;
#endif
	//dbgprintf("DIP:IOS_Ioctl( %d 0x%x 0x%p 0x%x 0x%p 0x%x )\n", msg->fd, msg->ioctl.command, bufin, lenin, bufout, lenout);
			
	switch(msg->ioctl.command)
	{
		case 0x23:
		{

#ifdef FAT
			f_close( &f );

			DIR d;
			if( f_opendir( &d, "/games" ) != FR_OK )
			{
				ret = DI_FATAL;
				break;
			}

			FILINFO FInfo;
			u32 count = 0;
			ret = DI_FATAL;
			while( f_readdir( &d, &FInfo ) == FR_OK )
			{
				if( count == *(u32*)(bufin+4) )
				{
					//build path
					sprintf( GamePath, "/games/%s/", FInfo.fname );
					dbgprintf("%s\n", GamePath );

					char *str = malloca( 32, 32 );
					sprintf( str, "%spart.bin", GamePath );

					if( f_open( &f, str, FA_READ ) == FR_OK )
					{
						Disc = 1;
						ret = DI_SUCCESS;
					} else {
						Disc = 0;
						ret = DI_FATAL;
					}

					free( str );
					break;
				}
				count++;
			}
#else
			SlotID = *(vu32*)(bufin+4);
			//quick test if there is a game
			u8 *mem = (u8*)heap_alloc_aligned( heapid, 32, 32 );
			ret = USB_Read( 0, 0x20, mem );
			if( ret == DI_SUCCESS )
			{
				if( *(vu32*)(mem+0x18) == 0x5D1C9EA3 ) 
					ret = DI_SUCCESS;
				else
					ret = DI_FATAL;
			}
			heap_free( heapid, mem );
#endif
			dbgprintf("DIP:DVDSelectGame(%d):%d\n", *(u32*)(bufin+4), ret );
		} break;
		case 0x96:
		case DVD_REPORTKEY:
		{
			ret = DI_ERROR;
			error = 0x00052000;
			dbgprintf("DIP:DVDLowReportKey():%d\n", ret );
		} break;
		case 0xDD:		// 0 out
		{
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowSetMaximumRotation():%d\n", ret);
		} break;
		case 0x95:		// 0x20 out
		{
			*(u32*)bufout = DIStatus;
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowPrepareStatusRegister( %08X ):%d\n", DIStatus, ret );
		} break;
		case 0x7A:		// 0x20 out
		{
			*(u32*)bufout = DICover;
			ret = DI_SUCCESS;
			//dbgprintf("DIP:DVDLowPrepareCoverRegister( %08X ):%d\n", DICover, ret );
		} break;
		case 0x86:		// 0 out
		{
			ret = DI_SUCCESS;
			//dbgprintf("DIP:DVDLowClearCoverInterrupt():%d\n", ret);
		} break;
		case DVD_GETCOVER:	// 0x88 - Cover is always closed
		{
			*(u32*)bufout = DICover;				// 1 is open
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDGetCover( %08X ):%d\n", DICover, ret );
		} break;
		case 0x89:
		{
			DICover &= ~4;
			DICover |= 2;

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowUnknownRegister():%d\n", ret );
		} break;
		case DVD_IDENTIFY:
		{
			memset( bufout, 0, 0x20 );

			*(u32*)(bufout)		= 0x00000002;
			*(u32*)(bufout+4)	= 0x20070213;
			*(u32*)(bufout+8)	= 0x41000000;

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowIdentify():%d\n", ret);
		} break;
		case DVD_GET_ERROR:	// 0xE0
		{
			*(u32*)bufout = error; 
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowGetError( %08X ):%d\n", error, ret );
		} break;
		case 0x8E:
		{
			if( (*(u8*)(bufin+4)) == 0 )
				EnableVideo( 1 );
			else
				EnableVideo( 0 );

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowEnableVideo(%d):%d\n", *(u32*)(bufin+4), ret);
		} break;
		case DVD_LOW_READ:	// 0x71
		{
			if( *(u32*)(bufin+8) > PartitionSize )
			{
				ret = DI_ERROR;
				error = 0x00052100;

			} else {
				if( Partition )
				{
#ifdef FAT
					s32 r=0;
					r = f_lseek( &f, ((*(u32*)(bufin+8))<<2) );
					if( r != FR_OK )
					{
						dbgprintf("f_lseek():%d\n", r);
						ret = DI_FATAL;
					} else {
						r = f_read( &f, bufout, ((*(u32*)(bufin+4))+31)&(~31), &read );
						if( r != FR_OK )
						{
							dbgprintf("f_read():%d\n", r);
							ret = DI_FATAL;
						} else {
							if( read < *(u32*)(bufin+4) )
							{
								dbgprintf("f_read(): %d < %d\n", read < *(u32*)(bufin+4));
								ret = DI_FATAL;
							} else
								ret = DI_SUCCESS;
						}
					}
#else
					ret = USB_EncryptedRead( (*(u32*)(bufin+8)), *(u32*)(bufin+4), bufout );
#endif
				} else {
					DIP_Fatal("DVDLowread", __LINE__, __FILE__, 64, "Partition not opened!");
				}
			}

			dbgprintf("DIP:DVDLowRead( %08X, %08X, %p ):%d\n", *(u32*)(bufin+8), *(u32*)(bufin+4), bufout, ret );
		} break;
		case DVD_READ_UNENCRYPTED:
		{
			if( *(u32*)(bufin+8) > 0x46090000 )
			{
				ret = DI_ERROR;
				error = 0x00052100;

			} else {
#ifdef FAT
				switch( *(u32*)(bufin+8) )
				{
					case 0x00:
					{
						f_lseek( &f, 0 );
						if( f_read( &f, bufout, *(u32*)(bufin+4), &read ) != FR_OK )
							ret = DI_FATAL;
						else 
							ret = DI_SUCCESS;
					} break;
					case 0x08:		// 0x20
					{
						f_lseek( &f, 0x20 );
						if( f_read( &f, bufout, *(u32*)(bufin+4), &read ) != FR_OK )
							ret = DI_FATAL;
						else 
							ret = DI_SUCCESS;
					} break;
					case 0x10000:		// 0x40000
					{
						memset( bufout, 0, lenout );

						*(u32*)(bufout)		= 1;				// one partition
						*(u32*)(bufout+4)	= 0x40020>>2;		// partition table info

						ret = DI_SUCCESS;
					} break;
					case 0x10008:		// 0x40020
					{
						memset( bufout, 0, lenout );

						*(u32*)(bufout)		= 0x03E00000;		// partition offset
						*(u32*)(bufout+4)	= 0x00000000;		// partition type

						ret = DI_SUCCESS;
					} break;
					case 0x00013800:		// 0x4E000
					{
						memset( bufout, 0, lenout );

						*(u32*)(bufout)			= 0x00000002;			// 0 = JAP, 1 = USA, 2 = EUR, 4 = KOR
						*(u32*)(bufout+0x1FFC)	= 0xC3F81A8E;

						ret = DI_SUCCESS;
					} break;
					default:
						dbgprintf("DIP:unexpected read offset!\n");
						dbgprintf("DIP:DVDLowUnencryptedRead( %08X, %08X, %p ):%d\n", *(u32*)(bufin+8), *(u32*)(bufin+4), bufout, ret );
						while(1);
					break;
				}
#else
				ret = USB_Read( *(u32*)(bufin+8)<<2, ((*(u32*)(bufin+4))+31)&(~31), bufout );

				//patch out update partition!
				if( *(u32*)(bufin+8) == 0x00010000 )
				{
					*(u32*)bufout = 1;
					PartitionTableOffset = *(u32*)(bufout+4);

				} else if ( (*(u32*)(bufin+8) == PartitionTableOffset) && (PartitionTableOffset != 0)  ) {

					*(u32*)(bufout) = *(u32*)(bufout+8); 
					*(u32*)(bufout+4) = 0; 
				}
#endif
			}

			dbgprintf("DIP:DVDLowUnencryptedRead( %08X, %08X, %p ):%d\n", *(u32*)(bufin+8), *(u32*)(bufin+4), bufout, ret );
		} break;
		case DVD_READ_DISCID:
		{
#ifdef FAT
			u8 *data = malloca( (lenout+31)&(~31), 0x40 );

			f_lseek( &f, 0 );
			ret = f_read( &f, data, lenout, &read );
			if( ret != FR_OK )
			{
				dbgprintf("fail:%d\n", ret );
				ret = DI_FATAL;
			} else {
				ret = DI_SUCCESS;
				memcpy( bufout, data, lenout );
			}
			
			free( data );

#else
			ret = USB_Read( 0, (lenout+31)&(~31), bufout );
			dbgprintf("DIP:SlotID:%d\n", SlotID );
#endif
			hexdump( bufout, lenout );


			dbgprintf("DIP:DVDLowReadDiscID(%p):%d\n", bufout, ret );
		} break;
		case DVD_RESET:
		{
			DIStatus = 0x00;
			DICover  = 0x00;

			Partition = 0;

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowReset( %d ):%d\n", *(u32*)(bufin+4), ret);
		} break;
		case DVD_SET_MOTOR:
		{
			Motor = 0;
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDStopMotor(%d,%d):%d\n", *(u32*)(bufin+4), *(u32*)(bufin+8), ret);
		} break;
		case DVD_CLOSE_PARTITION:
		{
			Partition=0;
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowClosePartition():%d\n", ret );
		} break;
		case DVD_READ_BCA:
		{
			memset32( (u32*)bufout, 0, 0x40 );
			*(u32*)(bufout+0x30) = 0x00000001;

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowReadBCA():%d\n", ret );
		} break;

		default:
			hexdump( bufin, lenin );
			dbgprintf("DIP:Unknown IOS_Ioctl( %d 0x%x 0x%p 0x%x 0x%p 0x%x )\n", msg->fd, msg->ioctl.command, bufin, lenin, bufout, lenout);
			mqueue_ack( (void *)msg, DI_FATAL);
			while(1);
		break;
	}

	//Reset errror after a successful command
	if( msg->command != 0xE0 && ret == DI_SUCCESS )
		error = 0;

	//if( ret == DI_SUCCESS && lenout )
	//	sync_before_read( bufout, lenout );

	mqueue_ack( (void *)msg, ret);
}
void DIP_Ioctlv(struct ipcmessage *msg)
{
	u32 InCount		= msg->ioctlv.argc_in;
	u32 OutCount	= msg->ioctlv.argc_io;
	vector *v		= (vector*)(msg->ioctlv.argv);
	s32 ret=0;

#ifdef FAT
	u32 read;
#endif
	switch(msg->ioctl.command)
	{
		case DVD_OPEN_PARTITION:
		{
			if( Partition == 1 )
			{
				DIP_Fatal( "DVDLowOpenPartition", __LINE__, __FILE__, 0, "Partition already open!");
			}

			PartitionOffset = *(u32*)(v[0].data+4);
			PartitionOffset <<= 2;

			u8 *TMD;
			u8 *TIK;
			u8 *CRT;
			u8 *hashes = (u8*)heap_alloc_aligned( heapid, sizeof(u8)*0x14, 32 );
			u8 *buffer = (u8*)heap_alloc_aligned( heapid, 0x40, 32 );
			memset( buffer, 0, 0x40 );

#ifdef FAT

			char *str = (char*)heap_alloc_aligned( heapid, 32,32 );

			sprintf( str, "%sticket.bin", GamePath );

			FIL fp;
			if( f_open( &fp, str, FA_READ ) == FR_OK )
			{
				TIK = (u8*)heap_alloc_aligned( heapid, 0x2a4, 32 );
				if( f_read( &fp, TIK, 0x2a4, &read ) != FR_OK )
				{
					ret = DI_FATAL;
					break;
				}

				((u32*)buffer)[0x04] = (u32)TIK;			//0x10
				((u32*)buffer)[0x05] = 0x2a4;				//0x14
				sync_before_read(TIK, 0x2a4);

				f_close( &fp );
			} else {
				dbgprintf("Could not open:\"%s\"\n", str );
				ret = DI_FATAL;
				break;

			}
			sprintf( str, "%stmd.bin", GamePath );

			if( f_open( &fp, str, FA_READ ) == FR_OK )
			{
				u32 asize = (fp.fsize+31)&(~31);
				TMD = (u8*)heap_alloc_aligned( heapid, asize, 32 );
				memset( TMD, 0, asize );
				if( f_read( &fp, TMD, fp.fsize, &read ) != FR_OK )
				{
					ret = DI_FATAL;
					break;
				} 

				((u32*)buffer)[0x06] = (u32)TMD;		//0x18
				((u32*)buffer)[0x07] = fp.fsize;		//0x1C
				sync_before_read(TMD, asize);

				PartitionSize = (u32)((*(u64*)(TMD+0x1EC)) >> 2 );
				dbgprintf("DIP:Set partition size to:%08X%08X\n", (((u64)PartitionSize)<<2)&0xFFFFFFFF00000000, PartitionSize&0xFFFFFFFF );

				if( v[3].data != NULL )
				{
					memcpy( v[3].data, TMD, fp.fsize );
					sync_after_write( v[3].data, fp.fsize );
				}

				f_close( &fp );
			} else {
				dbgprintf("Could not open:\"%s\"\n", str );
				ret = DI_FATAL;
				break;
			}

			sprintf( str, "%scert.bin", GamePath );
			if( f_open( &fp, str, FA_READ ) == FR_OK )
			{
				CRT = (u8*)heap_alloc_aligned( heapid, 0xA00, 32 );
				if( f_read( &fp, CRT, 0xA00, &read ) != FR_OK )
				{
					ret = DI_FATAL;
					break;
				}

				((u32*)buffer)[0x00] = (u32)CRT;		//0x00
				((u32*)buffer)[0x01] = 0xA00;			//0x04
				sync_before_read(CRT, 0xA00);

				f_close( &fp );
			} else {
				dbgprintf("Could not open:\"%s\"\n", str );
				ret = DI_FATAL;
				break;
			}

			heap_free( heapid, str );
#else
			PartitionInfo PI;
			USB_Read( PartitionOffset + 0x2A4, sizeof(PartitionInfo), &PI );

			PartitionDataOffset = (u64)(PI.DataOffset<<2) + PartitionOffset;

			dbgprintf("DIP:PartitionDataOffset:%x\n", (u32)(PartitionDataOffset>>2) );
			dbgprintf("DIP:PartitionOffset:%x\n", (u32)(PartitionOffset>>2) );

			dbgprintf("DIP:TMDSize:%d\n", PI.TMDSize );
			dbgprintf("DIP:TMDOffset:%X\n", PI.TMDOffset<<2 );

			dbgprintf("DIP:DataSize:%X\n", PI.DataSize );
			dbgprintf("DIP:DataOffset:%X\n", PI.DataOffset<<2 );

			dbgprintf("DIP:CertChainSize:%d\n", PI.CertChainSize );
			dbgprintf("DIP:CertChainOffset:%X\n", PI.CertChainOffset<<2 );

			PartitionSize = PI.DataSize;

			TIK = (u8*)heap_alloc_aligned( heapid, 0x2A4, 32 );
			USB_Read( PartitionOffset, 0x2A4, TIK );
			//dbgprintf("Read TIK:%d\n", USB_Read( PartitionOffset, 0x2A4, TIK ) );

			((u32*)buffer)[0x04] = (vu32)TIK;		//0x10
			((u32*)buffer)[0x05] = 0x2A4;			//0x14
			sync_after_write(TIK, 0x2A4);

			//hexdump( TIK, 0x2A4 );

			TMD = (u8*)heap_alloc_aligned( heapid, PI.TMDSize, 32 );
			USB_Read( PartitionOffset + (PI.TMDOffset<<2), PI.TMDSize, TMD );
 			//dbgprintf("Read TMD:%d\n", USB_Read( PartitionOffset + (PI.TMDOffset<<2), PI.TMDSize, TMD ) );

			((u32*)buffer)[0x06] = (vu32)TMD;		//0x18
			((u32*)buffer)[0x07] = PI.TMDSize;		//0x1C
			sync_after_write(TMD, PI.TMDSize);

			memcpy( (u8*)(v[3].data), TMD, PI.TMDSize );

			//hexdump( TMD, PI.TMDSize );

			CRT = (u8*)heap_alloc_aligned( heapid, PI.CertChainSize, 32 );
			USB_Read( PartitionOffset + (PI.CertChainOffset<<2), PI.CertChainSize, CRT );
 			//dbgprintf("Read CRT:%d\n", USB_Read( PartitionOffset + (PI.CertChainOffset<<2), PI.CertChainSize, CRT ) );

			((u32*)buffer)[0x00] = (vu32)CRT;		//0x00
			((u32*)buffer)[0x01] = PI.CertChainSize;//0x04
			sync_after_write(CRT, PI.CertChainSize);

#endif
			((u32*)buffer)[0x02] = (u32)NULL;			//0x08
			((u32*)buffer)[0x03] = 0;					//0x0C

			//out
			((u32*)buffer)[0x08] = (u32)KeyID;			//0x20
			((u32*)buffer)[0x09] = 4;					//0x24
			((u32*)buffer)[0x0A] = (u32)hashes;			//0x28
			((u32*)buffer)[0x0B] = 20;					//0x2C

			sync_before_read(buffer, 0x40);

			s32 ESHandle = IOS_Open("/dev/es", 0 );
			sync_before_read(ESHandle, 4);

			s32 r = ios_ioctlv( ESHandle, 0x1C, 4, 2, buffer );
			dbgprintf("ios_ioctlv( %d, %02X, %d, %d, %p ):%d\n", ESHandle, 0x1C, 4, 2, buffer, r );
			IOS_Close( ESHandle );

			dbgprintf("KeyID:%d\n", *KeyID );

			*(u32*)(v[4].data) = 0x00000000;

			heap_free( heapid, buffer );
			heap_free( heapid, TIK );
			heap_free( heapid, CRT );
			heap_free( heapid, TMD );

			//IOS_Close( fd );
			if( r < 0 )
			{
				DIP_Fatal( "DVDLowOpenPartition", __LINE__, __FILE__, r, "ios_ioctlv() failed!");
			}
			

			//dbgprintf("DIP:PartitionSize:%X\n", PartitionSize );

			ret = DI_SUCCESS;
			Partition=1;

			dbgprintf("DIP:DVDOpenPartition(0x%08X):%d\n", *(u32*)(v[0].data+4), ret );

		} break;
		default:
			dbgprintf("DIP:IOS_Ioctlv( %d, 0x%x 0x%x 0x%x 0x%p )\n", msg->fd, msg->ioctl.command, InCount, OutCount, msg->ioctlv.argv );
			mqueue_ack( (void *)msg, DI_FATAL);
			while(1);
		break;
	}

	//Reset errror after a successful command
	if( ret == DI_SUCCESS )
		error = 0;

	mqueue_ack( (void *)msg, ret);
}
s32 RegisterDevices( void )
{
	queueid = mqueue_create(queuespace, 8);

	s32 ret = device_register("/dev/di", queueid);
#ifdef DEBUG
	dbgprintf("DIP:DeviceRegister(\"/dev/di\"):%d\n", ret );
#endif
	if( ret < 0 )
		return ret;

	return queueid;
}

char __aeabi_unwind_cpp_pr0[0];
void _main(void)
{
	thread_set_priority( 0, 0xF4 );

#ifdef DEBUG
	dbgprintf("$IOSVersion: DIP: %s %s 64M DEBUG$\n", __DATE__, __TIME__ );
#else
	dbgprintf("$IOSVersion: DIP: %s %s 64M Release$\n", __DATE__, __TIME__ );
#endif

	if( syscall_04() != 3 || syscall_03() != 7 )
	{
		dbgprintf("PID:%d TID:%d\n", syscall_03(), syscall_04() );
		DIP_Fatal("main()", __LINE__, __FILE__, 0, "PID must be 7 and TID must be 3!");
	}

	s32 ret=0;
	struct ipcmessage *message=NULL;

	heapid = heap_create( (void*)0x13600000, 0x18000 );
	queuespace = heap_alloc(heapid, 0x20);
	queueid = RegisterDevices();
	if( queueid < 0 )
	{
		ThreadCancel( 0, 0x77 );
	}
	
	ret = EnableVideo(1);
	dbgprintf("DIP: EnableVideo(1):%d\n", ret );

	IRQ_Enable_18();
#ifdef FAT
	s32 fres = f_mount(0, &fatfs);
	dbgprintf("DIP:f_mount():%d\n", fres);

	if( fres != FR_OK )
	{
		DIP_Fatal("main()", __LINE__, __FILE__, 0, "Could not find any USB device!");
		ThreadCancel( 0, 0x77 );
	}

	fres = f_open( &f, "/games/SMNE/part.bin", FA_READ );
	sprintf( GamePath, "/games/SMNE/" );


	if( fres != FR_OK )
	{
		DIP_Fatal("main()",__LINE__,__FILE__, fres, "Failed to open part.bin!");
		while(1);
	}

#else

	udelay( 50000 );

	dbgprintf("DIP: Initializing TinyEHCI...\n");
	tiny_ehci_init();

	u32 s_cnt = 0;
	dbgprintf("DIP: Discovering EHCI devices...\n");

	while( ehci_discover() == -ENODEV )
		udelay( 4000 );

	dbgprintf("done\n");
	
	USBStorage_Init();
	
	u32 s_size;
	s_cnt = USBStorage_Get_Capacity(&s_size);
	
	dbgprintf("DIP: Drive size: %dMB SectorSize:%d\n", s_cnt / 1024 * s_size / 1024, s_size);

#endif

	error = 0;

	KeyID = malloca( sizeof( u32 ), 0x40 );
	*KeyID = 0;

#ifndef FAT
	SlotID=0;
#endif

	DICover = 2;
	DIStatus= 0;

	while (1)
	{
		ret = mqueue_recv(queueid, (void *)&message, 0);
		if( ret != 0 )
		{
			dbgprintf("DIP: mqueue_recv(%d) FAILED:%d\n", queueid, ret);
			continue;
		}

		switch( message->command )
		{
			case IOS_OPEN:
			{
				
				if( strncmp("/dev/di", message->open.device, 8 ) == 0 )
				{
					dbgprintf("DIP:IOS_Open(\"%s\"):%d\n", message->open.device, 24 );
					mqueue_ack( (void *)message, 24 );
				} else {
					dbgprintf("DIP:IOS_Open(\"%s\"):%d\n", message->open.device, -6 );
					mqueue_ack( (void *)message, -6 );
				}
				
			} break;

			case IOS_CLOSE:
			{
				dbgprintf("DIP:IOS_Close(%d)\n", message->fd );
				mqueue_ack( (void *)message, 0);
			} break;

			case IOS_IOCTL:
				DIP_Ioctl( message );
			break;

			case IOS_IOCTLV:
				DIP_Ioctlv( message );
				break;

			case IOS_READ:
			case IOS_WRITE:
			case IOS_SEEK:
			default:
				dbgprintf("unimplemented/invalid msg: %08x\n", message->command);
				mqueue_ack((void *)message, -4);
				while(1);
		}
	}
}

