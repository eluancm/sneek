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
#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "common.h"
#include "ES.h"
#include "sdhcreg.h"
#include "sdmmcreg.h"
#include "alloc.h"

int verbose = 0;
u32 base_offset=0;
static char heap[0x100] ALIGNED(32);
void *queuespace=NULL;
int queueid = 0;
int heapid=0;
int FFSHandle=0;

#undef DEBUG

static u32 SkipContent ALIGNED(32);
static u64 TitleID ALIGNED(32);
static u32 KernelVersion ALIGNED(32);
static u8 *iTMD=NULL;			//used for information during title import
static u8 *iTIK=NULL;			//used for information during title import
extern u32 *KeyID;
extern u8 *CNTMap;

typedef struct {
	u32 command;
	u32 type;				// 11b ABORT, 10b RESUME, 01b SUSPEND, 00b NORMAL
	u32 rep;			// 11b 48bytes Check Busy, 10b 48bytes, 01b 136bytes, 00b NONE
	u32 arg;			// RW
	u32 blocks;
	u32 bsize;
	u32 addr;
	u32 isDMA;
	u32 pad0;
} SDCommand;


	u8 rawData[20] =
	{
		0x93, 0x4D, 0xB1, 0x0D,
		0xAC, 0xFE, 0x4F, 0x41,
		0x0D, 0x83, 0xE8, 0xE6,
		0x45, 0x67, 0x67, 0xB6, 
		0x8B, 0xD9, 0x1A, 0x7A, 
	} ;

static u32 *HCR;
static u32 *SDStatus;		//0x00110001
static u32 SDClock=0;

u8 value[] = { "f00dcafe" };

void iCleanUpTikTMD( void )
{
	if( iTMD != NULL )
	{
		free( iTMD );
		iTMD = NULL;
	}
	if( iTIK != NULL )
	{
		free( iTIK );
		iTIK = NULL;
	}
}
void SD_Ioctl( struct ipcmessage *msg )
{
	u8  *bufin  = (u8*)msg->ioctl.buffer_in;
	u32 lenin   = msg->ioctl.length_in;
	u8  *bufout = (u8*)msg->ioctl.buffer_io;
	u32 lenout  = msg->ioctl.length_io;
	s32 ret = -1;

	switch( msg->ioctl.command )
	{
		case 0x01:	// Write HC Register
		{
			u32 reg = *(u32*)(bufin);
			u32 val = *(u32*)(bufin+16);

			if( (reg==0x2C) && (val==1) )
			{
				HCR[reg] = val | 2;
			} else if( (reg==0x2F) && val )
			{
				HCR[reg] = 0;
			} else {
				HCR[reg] = val;
			}

			ret = 0;
			dbgprintf("SD:SetHCRegister(%02X:%02X):%d\n", reg, HCR[reg], ret );
		} break;
		case 0x02:	// Read HC Register
		{
			*(u32*)(bufout) = HCR[*(u32*)(bufin)];

			ret = 0;
			dbgprintf("SD:GetHCRegister(%02X:%02X):%d\n", *(u32*)(bufin), HCR[*(u32*)(bufin)], ret );
		} break;
		case 0x04:	// sd_reset_card
		{
			*SDStatus |= 0x10000;
			*(u32*)(bufout) = 0x9f620000;

			ret = 0;
			//dbgprintf("SD:Reset(%08x):%d\n", *(u32*)(bufout), ret);
		} break;
		case 0x06:
		{
			SDClock = *(u32*)(bufin);
			ret=0;
			dbgprintf("SD:SetClock(%d):%d\n", *(u32*)(bufin), ret );
		} break;
		case 0x07:
		{
			SDCommand *scmd = (SDCommand *)(bufin);
			memset32( bufout, 0, 0x10 );

			switch( scmd->command )
			{
				case 0:
					ret = 0;
					dbgprintf("SD:GoIdleState():%d\n", ret);
				break;
				case 3:
					*(u32*)(bufout) = 0x9f62;
					ret = 0;
					dbgprintf("SD:SendRelAddress():%d\n", ret);
				break;
				case SD_APP_SET_BUS_WIDTH:		// 6	ACMD_SETBUSWIDTH
				{
					*(u32*)(bufout) = 0x920;
					ret=0;
					dbgprintf("SD:SetBusWidth(%d):%d\n", scmd->arg, ret );
				} break;
				case MMC_SELECT_CARD:			// 7
				{
					if( scmd->arg >> 16 )
						*(u32*)(bufout) = 0x700;
					else
						*(u32*)(bufout) = 0x900;

					ret=0;
					dbgprintf("SD:SelectCard(%08x):%d\n", *(u32*)(bufout), ret);
				} break;
				case 8:
				{
					*(u32*)(bufout) = scmd->arg;
					ret=0;
					dbgprintf("SD:SendIFCond(%08x):%d\n", *(u32*)(bufout), ret);
				} break;
				case MMC_SEND_CSD:
				{
					*(u32*)(bufout)		= 0x80168000;
					*(u32*)(bufout+4)	= 0xa9ffffff;
					*(u32*)(bufout+8)	= 0x325b5a83;
					*(u32*)(bufout+12)	= 0x00002e00;
					ret=0;
					dbgprintf("SD:SendCSD():%d\n", ret );
				} break;
				case MMC_ALL_SEND_CID:	// 2
				case 10:				// SEND_CID
				{
					*(u32*)(bufout)		= 0x80114d1c;
					*(u32*)(bufout+4)	= 0x80080000;
					*(u32*)(bufout+8)	= 0x8007b520;
					*(u32*)(bufout+12)	= 0x80080000;			
					ret=0;
					dbgprintf("SD:SendCID():%d\n", ret );
				} break;
				case MMC_SET_BLOCKLEN:	// 16 0x10
				{
					*(u32*)(bufout) = 0x900;
					ret=0;
					dbgprintf("SD:SetBlockLen(%d):%d\n", scmd->arg, ret );
				} break;
				case MMC_APP_CMD:	// 55 0x37
				{
					*(u32*)(bufout) = 0x920;
					ret=0;
					dbgprintf("SD:AppCMD(%08x):%d\n", *(u32*)(bufout), ret);
				} break;
				case SDHC_CAPABILITIES:
				{
					//0x01E130B0
					if( (*SDStatus)&0x10000 )
						*(u32*)(bufout) = (1 << 7) | (63 << 8);
					else
						*(u32*)(bufout) = 0;

					ret=0;
					//dbgprintf("SD:GetCapabilities():%d\n", ret );
				} break;
				case 0x41:
				{
					*SDStatus &= ~0x10000;
					ret=0;
					dbgprintf("SD:Unmount(%02X):%d\n", scmd->command, ret );
				} break;
				case 4:
				{
					ret=0;
					dbgprintf("SD:Command(%02X):%d\n", scmd->command, ret );
				} break;
				case 12:	// STOP_TRANSMISSION
				{
					dbgprintf("SD:StopTransmission()\n");
				} break;
				case SDHC_POWER_CTL:
				{
					*(u32*)(bufout) = 0x80ff8000;
					ret=0;
					dbgprintf("SD:SendOPCond(%04X):%d\n", *(u32*)(bufout), ret);
				} break;
				case 0x19:	// CMD25 WRITE_MULTIPLE_BLOCK
				{
					vector *vec = (vector *)malloca( sizeof(vector), 0x40 );

					vec[0].data = (u32)bufin;
					vec[0].len = sizeof(SDCommand);

					ret = IOS_Ioctlv( FFSHandle, 0x21, 1, 0, vec );

					if( ret < 0 )
					{
						SDCommand *scmd = (SDCommand *)bufin;
						dbgprintf("SD:WriteMultipleBlock( 0x%p, 0x%x, 0x%x):%d\n", scmd->addr, scmd->arg, scmd->blocks, ret );
					}
					free( vec );

				} break;
				default:
				{
					dbgprintf("Command:%08X\n", *(u32*)(bufin) );
					dbgprintf("CMDType:%08X\n", *(u32*)(bufin+4) );
					dbgprintf("ResType:%08X\n", *(u32*)(bufin+8) );
					dbgprintf("Argumen:%08X\n", *(u32*)(bufin+0x0C) );
					dbgprintf("BlockCn:%08X\n", *(u32*)(bufin+0x10) );
					dbgprintf("SectorS:%08X\n", *(u32*)(bufin+0x14) );
					dbgprintf("Buffer :%08X\n", *(u32*)(bufin+0x18) );
					dbgprintf("unkown :%08X\n", *(u32*)(bufin+0x1C) );
					dbgprintf("unknown:%08X\n", *(u32*)(bufin+0x20) );

					dbgprintf("Unhandled command!\n");
				} break;
			}

			//dbgprintf("SD:Command(0x%02X):%d\n", *(u32*)(bufin), ret );
		} break;
		case 0x0B:	// sd_get_status
		{
			*(u32*)(bufout) = *SDStatus;
			ret = 0;
			//dbgprintf("SD:GetStatus(%08X):%d\n", *(u32*)(bufout), ret);
		} break;
		case 0x0C:	//OCRegister
		{
			*(u32*)(bufout) = 0x80ff8000;
			ret = 0;
			dbgprintf("SD:GetOCRegister(%08X):%d\n", *(u32*)(bufout), ret);
		} break;
		default:
			ret = -1;
			dbgprintf("SD:IOS_Ioctl( %d 0x%x 0x%p 0x%x 0x%p 0x%x )\n", msg->fd, msg->ioctl.command, bufin, lenin, bufout, lenout);
			break;
	}

	mqueue_ack( (void *)msg, ret);
}
void SD_Ioctlv( struct ipcmessage *msg )
{
	u32 InCount		= msg->ioctlv.argc_in;
	u32 OutCount	= msg->ioctlv.argc_io;
	vector *v		= (vector*)(msg->ioctlv.argv);
	s32 ret=-1017;
	u32 i;

	switch(msg->ioctl.command)
	{
		case 0x07:
		{
			ret = IOS_Ioctlv( FFSHandle, 0x20, msg->ioctlv.argc_in, msg->ioctlv.argc_io, msg->ioctlv.argv );
			memset32( (u32*)(v[2].data), 0, v[2].len );

			if( ret < 0 )
			{
				SDCommand *scmd = (SDCommand *)(v[0].data);
				dbgprintf("SD:ReadMultipleBlocks( 0x%p, 0x%x, 0x%x):%d\n", scmd->addr, scmd->arg, scmd->blocks, ret );

				dbgprintf("cmd    :%08X\n", scmd->command );
				dbgprintf("type   :%08X\n", scmd->type );
				dbgprintf("resp   :%08X\n", scmd->rep );
				dbgprintf("arg    :%08X\n", scmd->arg );
				dbgprintf("blocks :%08X\n", scmd->blocks );
				dbgprintf("bsize  :%08X\n", scmd->bsize );
				dbgprintf("addr   :%08X\n", scmd->addr );
				dbgprintf("isDMA  :%08X\n", scmd->isDMA );
			}
		} break;
		default:
			for( i=0; i<InCount+OutCount; ++i)
			{
				dbgprintf("data:%p len:%d(0x%X)\n", v[i].data, v[i].len, v[i].len );
			}
			dbgprintf("SD:IOS_Ioctlv( %d 0x%x %d %d 0x%p )\n", msg->fd, msg->ioctlv.command, msg->ioctlv.argc_in, msg->ioctlv.argc_io, msg->ioctlv.argv);
			while(1);
		break;
	}

	mqueue_ack( (void *)msg, ret);

}
void ES_Ioctlv( struct ipcmessage *msg )
{
	u32 InCount		= msg->ioctlv.argc_in;
	u32 OutCount	= msg->ioctlv.argc_io;
	vector *v		= (vector*)(msg->ioctlv.argv);
	s32 ret=-1017;
	u32 i;

	//dbgprintf("ES:IOS_Ioctlv( %d 0x%x %d %d 0x%p )\n", msg->fd, msg->ioctlv.command, msg->ioctlv.argc_in, msg->ioctlv.argc_io, msg->ioctlv.argv);
	//	
	//for( i=0; i<InCount+OutCount; ++i)
	//{
	//	dbgprintf("data:%p len:%d\n", v[i].data, v[i].len );
	//}

	switch(msg->ioctl.command)
	{
		case IOCTL_ES_VERIFYSIGN:
		{
			ret = ES_SUCCESS;
			dbgprintf("ES:VerifySign():%d\n", ret );		
		} break;
		case IOCTL_ES_DECRYPT:
		{
			ret = aes_decrypt_( *(u32*)(v[0].data), (u8*)(v[1].data), (u8*)(v[2].data), v[2].len, (u8*)(v[4].data));
			dbgprintf("ES:Decrypt( %d, %p, %p, %d, %p ):%d\n", *(u32*)(v[0].data), (u8*)(v[1].data), (u8*)(v[2].data), v[2].len, (u8*)(v[4].data), ret );	
		} break;
		case IOCTL_ES_ENCRYPT:
		{
			ret = aes_encrypt( *(u32*)(v[0].data), (u8*)(v[1].data), (u8*)(v[2].data), v[2].len, (u8*)(v[4].data));
			dbgprintf("ES:Encrypt( %d, %p, %p, %d, %p ):%d\n", *(u32*)(v[0].data), (u8*)(v[1].data), (u8*)(v[2].data), v[2].len, (u8*)(v[4].data), ret );	
		} break;
		case IOCTL_ES_SIGN:
		{
			ret = ES_Sign( &TitleID, (u8*)(v[0].data), v[0].len, (u8*)(v[1].data), (u8*)(v[2].data) );
			dbgprintf("ES:Sign():%d\n", ret );		
		} break;
		case IOCTL_ES_GETDEVICECERT:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );

			_sprintf( path, "/sys/device.cert" );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				GetDeviceCert( (u8*)(v[0].data) );
			} else {
				memcpy32( (u8*)(v[0].data), data, 0x180 );
				free( data );
			}

			free( size );
			free( path );

			ret = ES_SUCCESS;
			dbgprintf("ES:GetDeviceCert():%d\n", ret );			
		} break;
		case IOCTL_ES_DIGETSTOREDTMD:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)(TitleID) );

			u8 *data = NANDLoadFile( path, size );
			if( data != NULL )
			{
				memcpy32( (u8*)(v[1].data), data, *size );
				
				ret = ES_SUCCESS;
				free( data );

			} else {
				ret = *size;
			}

			free( size );
			free( path );

			dbgprintf("ES:DIGetStoredTMD( %08x-%08x ):%d\n", (u32)(TitleID>>32), (u32)(TitleID), ret );
		} break;
		case IOCTL_ES_DIGETSTOREDTMDSIZE:
		{
			char *path = (char*)malloca( 0x40, 32 );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)(TitleID) );

			s32 fd = IOS_Open( path, 1 );
			if( fd < 0 )
			{
				ret = fd;
			} else {

				fstats *status = (fstats*)malloc( sizeof(fstats) );
				ret = ISFS_GetFileStats( fd, status );
				if( ret < 0 )
				{
					dbgprintf("ES:ISFS_GetFileStats(%d, %p ):%d\n", fd, status, ret );
					free( status );
				} else
					*(u32*)(v[0].data) = status->Size;
			}

			dbgprintf("ES:DIGetStoredTMDSize( %08x-%08x ):%d\n", (u32)(TitleID>>32), (u32)(TitleID), ret );
			
		} break;
		case IOCTL_ES_GETSHAREDCONTENTS:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );

			_sprintf( path, "/shared1/content.map" );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = ES_FATAL;
			} else {

				for( i=0; i < *(u32*)(v[0].data); ++i )
					memcpy32( (u8*)(v[1].data+i*20), data+i*0x1C+8, 20 );

				free( data );
				ret = ES_SUCCESS;
			}

			free( size );
			free( path );

			dbgprintf("ES:ES_GetSharedContents():%d\n", ret );
		} break;
		case IOCTL_ES_GETSHAREDCONTENTCNT:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );

			_sprintf( path, "/shared1/content.map" );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = ES_FATAL;
			} else {

				*(u32*)(v[0].data) = *size / 0x1C;

				free( data );
				ret = ES_SUCCESS;
			}

			free( size );
			free( path );

			dbgprintf("ES:ES_GetSharedContentCount(%d):%d\n", *(vu32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_GETTMDCONTENTCNT:
		{
			char *path = (char*)malloca( 0x40, 32 );

			*(u32*)(v[1].data) = 0;
		
			for( i=0; i < *(u16*)(v[0].data+0x1DE); ++i )
			{	
				if( (*(u16*)(v[0].data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
				{
					if( ES_CheckS1Conent( (u8*)(v[0].data+0x1F4+i*0x24) ) == 1 )
						(*(u32*)(v[1].data))++;
				} else {
					_sprintf( path, "/title/%08x/%08x/content/%08x.app", *(u32*)(v[0].data+0x18C), *(u32*)(v[0].data+0x190), *(u32*)(v[0].data+0x1E4+i*0x24) );
					s32 fd = IOS_Open( path, 1 );
					if( fd >= 0 )
					{
						(*(u32*)(v[1].data))++;
						IOS_Close( fd );
					}
				}
			}

			free( path );

			ret = ES_SUCCESS;
			dbgprintf("ES:GetTmdContentsOnCardCount(%d):%d\n", *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_GETTMDCONTENTS:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 count=0;

			for( i=0; i < *(u16*)(v[0].data+0x1DE); ++i )
			{	
				if( (*(u16*)(v[0].data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
				{
					if( ES_CheckS1Conent( (u8*)(v[0].data+0x1F4+i*0x24) ) == 1 )
					{
						*(u32*)(v[2].data+4*count) = *(u32*)(v[0].data+0x1E4+i*0x24);
						count++;
					}
				} else {
					_sprintf( path, "/title/%08x/%08x/content/%08x.app", *(u32*)(v[0].data+0x18C), *(u32*)(v[0].data+0x190), *(u32*)(v[0].data+0x1E4+i*0x24) );
					s32 fd = IOS_Open( path, 1 );
					if( fd >= 0 )
					{
						*(u32*)(v[2].data+4*count) = *(u32*)(v[0].data+0x1E4+i*0x24);
						count++;
						IOS_Close( fd );
					}
				}
			}

			free( path );

			ret = ES_SUCCESS;
			dbgprintf("ES:ListTmdContentsOnCard():%d\n", ret );
		} break;
		case IOCTL_ES_GETDEVICEID:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );

			_sprintf( path, "/sys/device.cert" );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				GetKey( 1, (u8*)(v[0].data) );
			} else {
			
				u32 value = 0;

				//Convert from string to value
				for( i=0; i < 8; ++i )
				{
					if( *(u8*)(data+i+0xC6) > '9' )
						value |= ((*(u8*)(data+i+0xC6))-'W')<<((7-i)<<2);	// 'a'-10 = 'W'
					 else 
						value |= ((*(u8*)(data+i+0xC6))-'0')<<((7-i)<<2);
				}

				*(u32*)(v[0].data) = value;
			}

			free( size );
			free( path );
			free( data );

			ret = ES_SUCCESS;

			dbgprintf("ES:ES_GetDeviceID( 0x%08x ):%d\n", *(u32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_GETCONSUMPTION:
		{
			*(u32*)(v[2].data) = 0;
			ret = ES_SUCCESS;

			dbgprintf("ES:ES_GetConsumption():%d\n", ret );
		} break;
		case IOCTL_ES_ADDTITLEFINISH:
		{
			ret = ES_AddTitleFinish( iTMD );

			//Get TMD for the CID names and delete!
			char *path = (char*)malloca( 0x40, 32 );

			_sprintf( path, "/tmp/title.tmd" );
			ISFS_Delete( path );

			for( i=0; i < *(u16*)(iTMD+0x1DE); ++i )
			{
				_sprintf( path, "/tmp/%08x", *(u32*)(iTMD+0x1E4+i*0x24) );
				ISFS_Delete( path );
				_sprintf( path, "/tmp/%08x.app", *(u32*)(iTMD+0x1E4+i*0x24) );
				ISFS_Delete( path );
			}

			free( path );

			////Delete contents from old versions of this title
			//if( ret == ES_SUCCESS )
			//{
			//	//get dir list
			//	_sprintf( path, "/ticket/%08x/%08x/content", *(u32*)(iTMD+0x18C), *(u32*)(iTMD+0x190) );

			//	ISFS_ReadDir( path, NULL, FileCount

			//}

			iCleanUpTikTMD();

			dbgprintf("ES:AddTitleFinish():%d\n", ret );
		} break;
		case IOCTL_ES_ADDTITLECANCEL:
		{
			//Get TMD for the CID names and delete!
			char *path = (char*)malloca( 0x40, 32 );

			_sprintf( path, "/tmp/title.tmd" );
			ISFS_Delete( path );

			for( i=0; i < *(u16*)(iTMD+0x1DE); ++i )
			{
				_sprintf( path, "/tmp/%08x", *(u32*)(iTMD+0x1E4+i*0x24) );
				ISFS_Delete( path );
				_sprintf( path, "/tmp/%08x.app", *(u32*)(iTMD+0x1E4+i*0x24) );
				ISFS_Delete( path );
			}

			free( path );

			iCleanUpTikTMD();

			ret = ES_SUCCESS;
			dbgprintf("ES:AddTitleCancel():%d\n", ret );
		} break;
		case IOCTL_ES_ADDCONTENTFINISH:
		{
			if( SkipContent )
			{
				ret = ES_SUCCESS;
				dbgprintf("ES:AddContentFinish():%d\n", ret );
				break;
			}

			char *path = (char*)malloca( 0x40, 32 );
			u32	 *size = (u32*)malloca( sizeof(u32), 32 );

			if( iTMD == NULL )
				ret = ES_FATAL;
			else {
				//load Ticket to forge the decryption key
				_sprintf( path, "/ticket/%08x/%08x.tik", *(u32*)(iTMD+0x18C), *(u32*)(iTMD+0x190) );

				iTIK = NANDLoadFile( path, size );
				if( iTIK == NULL )
				{
					iCleanUpTikTMD();
					ret = ES_ETIKTMD;

				} else {

					ret = ES_CreateKey( iTIK );
					if( ret >= 0 )
					{
						ret = ES_AddContentFinish( *(vu32*)(v[0].data), iTIK, iTMD );
						DestroyKey( *KeyID );
					}

					free( iTIK );
				}
			}

			free( size );
			free( path );

			dbgprintf("ES:AddContentFinish():%d\n", ret );
		} break;
		case IOCTL_ES_ADDCONTENTDATA:
		{
			if( SkipContent )
			{
				ret = ES_SUCCESS;
				dbgprintf("ES:AddContentData():%d\n", ret );
				break;
			}

			if( iTMD == NULL )
				ret = ES_FATAL;
			else {
				ret = ES_AddContentData( *(s32*)(v[0].data), (u8*)(v[1].data), v[1].len );
			}

			if( ret < 0 )
				dbgprintf("ES:AddContentData():%d\n", ret );
		} break;
		case IOCTL_ES_ADDCONTENTSTART:
		{
			SkipContent=0;

			if( iTMD == NULL )
				ret = ES_FATAL;
			else {
				//check if shared content and if it is already installed so we can skip this one
				for( i=0; i<*(u16*)(iTMD+0x1DE); ++i )
				{
					if( *(u32*)(iTMD+0x1E4+i*0x24) == *(u32*)(v[1].data) )
					{
						if( (*(u16*)(iTMD+0x1EA+i*0x24) & 0x8000) == 0x8000 )
						{
							if( ES_CheckS1Conent( (u8*)(iTMD+0x1F4+i*0x24) ) == 1 )
							{
								SkipContent=1;
								dbgprintf("ES:Content already installed, using fast install!\n");
							}
						}
						break;
					}
				}

				ret = *(u32*)(v[1].data);
			}

			dbgprintf("ES:AddContentStart():%d\n", ret );
		} break;
		case IOCTL_ES_ADDTITLESTART:
		{
			char *path = (char*)malloca( 0x40, 32 );

			//Copy TMD to internal buffer for later use
			iTMD = (u8*)malloca( v[0].len, 32 );
			memcpy32( iTMD, (u8*)(v[0].data), v[0].len );

			_sprintf( path, "/tmp/title.tmd" );

			ret = ISFS_CreateFile( path, 0, 3, 3, 3 );
			if( ret < 0 )
			{
				dbgprintf("ISFS_CreateFile(\"%s\"):%d\n", path, ret );
			} else {

				s32 fd = IOS_Open( path, 2 );
				if( fd < 0 )
				{
					dbgprintf("IOS_Open(\"%s\"):%d\n", path, fd );
					ret = fd;
				} else {
					ret = IOS_Write( fd, (u8*)(v[0].data), v[0].len );
					if( ret < 0 || ret != v[0].len )
					{
						dbgprintf("IOS_Write( %d, %p, %d):%d\n", fd, v[0].data, v[0].len, ret );
					} else {
						ret = ES_SUCCESS;
					}

					IOS_Close( fd );

					_sprintf( path, "/title/%08x/%08x/content", *(vu32*)(v[0].data+0x18C), *(vu32*)(v[0].data+0x190) );

					//Check if folder exists
					switch( ISFS_GetUsage( path, NULL, NULL ) )
					{
						case FS_ENOENT2:
						{
							//Create folders!
							_sprintf( path, "/title/%08x", *(vu32*)(v[0].data+0x18C) );
							if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
							{
								ret = ISFS_CreateDir( path, 0, 3, 3, 3 );
								if( ret < 0 )
								{
									dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, ret );
									break;
								}
							}

							_sprintf( path, "/title/%08x/%08x", *(vu32*)(v[0].data+0x18C), *(vu32*)(v[0].data+0x190) );
							if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
							{
								ret = ISFS_CreateDir( path, 0, 3, 3, 3 );
								if( ret < 0 )
								{
									dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, ret );
									break;
								}
							}

							_sprintf( path, "/title/%08x/%08x/content", *(vu32*)(v[0].data+0x18C), *(vu32*)(v[0].data+0x190) );
							if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
							{
								ret = ISFS_CreateDir( path, 0, 3, 3, 3 );
								if( ret < 0 )
								{
									dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, ret );
									break;
								}
							}
							_sprintf( path, "/title/%08x/%08x/data", *(vu32*)(v[0].data+0x18C), *(vu32*)(v[0].data+0x190) );
							if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
							{
								ret = ISFS_CreateDir( path, 0, 3, 3, 3 );
								if( ret < 0 )
								{
									dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, ret );
									break;
								}
							}

						} break;
						case FS_SUCCESS:
							ret = ES_SUCCESS;
							break;
						default:
							ret = ES_FATAL;
						break;
					}
				}
			}

			if( ret == ES_SUCCESS )
			{
				//Add new TitleUID to uid.sys
				u16 UID=0;
				ES_GetUID( (u64*)(iTMD+0x18C), &UID );
			}

			free( path );
			
			dbgprintf("ES:AddTitleStart():%d\n", ret );
		} break;
		case IOCTL_ES_ADDTICKET:
		{
			char *path = (char*)malloca( 0x40, 32 );
			memset32( path, 0, 0x40 );
			
			_sprintf( path, "/tmp/%08x.tik", *(vu32*)(v[0].data+0x01E0) );

			//Copy ticket to local buffer
			u8 *ticket = (u8*)malloca( v[0].len, 32 );
			
			memcpy8( ticket, (u8*)(v[0].data), v[0].len );

//check for console ID
			if( *(vu32*)(ticket+0x1D8) )
				doTicketMagic( ticket );

			ret = ISFS_CreateFile( path, 0, 3, 3, 3 );
			if( ret < 0 )
			{
				dbgprintf("ISFS_CreateFile(\"%s\"):%d\n", path, ret );
			} else {

				s32 fd = IOS_Open( path, 2 );
				if( fd < 0 )
				{
					dbgprintf("IOS_Open(\"%s\"):%d\n", path, fd );
					ret = fd;
				} else {

					ret = IOS_Write( fd, ticket, v[0].len );
					if( ret < 0 || ret != v[0].len )
					{
						dbgprintf("IOS_Write( %d, %p, %d):%d\n", fd, ticket, v[0].len, ret );
					} else {
						ret = ES_SUCCESS;
					}

					IOS_Close( fd );

					_sprintf( path, "/ticket/%08x", *(vu32*)(ticket+0x01dc) );

					//Check if folder exists
					switch( ISFS_GetUsage( path, NULL, NULL ) )
					{
						case FS_ENOENT2:
						{
							//Create folder!
							ret = ISFS_CreateDir( path, 0, 3, 3, 3 );
							if( ret < 0 )
							{
								dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, ret );
								break;
							}
						} /* !! fallthrough !! */
						case FS_SUCCESS:
						{
							char *dstpath = (char*)malloca( 0x40, 32 );

							_sprintf( path, "/tmp/%08x.tik", *(vu32*)(ticket+0x01E0) );
							_sprintf( dstpath, "/ticket/%08x/%08x.tik", *(vu32*)(ticket+0x01dc), *(vu32*)(ticket+0x01E0) );

							//this function moves the file, overwriting the target
							ret = ISFS_Rename( path, dstpath );
							if( ret < 0 )
								dbgprintf("ISFS_Rename( \"%s\", \"%s\" ):%d\n", path, dstpath, ret );

							free( dstpath );

						} break;
						default:
							ret = ES_FATAL;
						break;
					}
				}
			}

			_sprintf( path, "/title/%08x/%08x/content", *(vu32*)(ticket+0x01dc), *(vu32*)(ticket+0x01E0) );

			//Check if folder exists
			switch( ISFS_GetUsage( path, NULL, NULL ) )
			{
				case FS_ENOENT2:
				{
					//Create folders!
					_sprintf( path, "/title/%08x", *(vu32*)(ticket+0x01dc) );
					if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
					{
						ret = ISFS_CreateDir( path, 0, 3, 3, 3 );
						if( ret < 0 )
						{
							dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, ret );
							break;
						}
					}

					_sprintf( path, "/title/%08x/%08x", *(vu32*)(ticket+0x01dc), *(vu32*)(ticket+0x01E0) );
					if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
					{
						ret = ISFS_CreateDir( path, 0, 3, 3, 3 );
						if( ret < 0 )
						{
							dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, ret );
							break;
						}
					}
				} break;
				case FS_SUCCESS:
				{
					ret = ES_SUCCESS;
				} break;
				default:
					ret = ES_FATAL;
				break;
			}

			free( ticket );
			free( path );
			
			dbgprintf("ES:AddTicket(%08x-%08x):%d\n", *(vu32*)(v[0].data+0x01dc), *(vu32*)(v[0].data+0x01E0), ret );
		} break;
		case IOCTL_ES_EXPORTTITLEINIT:
		{
			ret = ES_SUCCESS;
			dbgprintf("ES:ExportTitleStart(%08x-%08x):%d\n", (u32)((*(u64*)(v[0].data))>>32), (u32)(*(u64*)(v[0].data)), ret );
		} break;
		case IOCTL_ES_EXPORTTITLEDONE:
		{
			ret = ES_SUCCESS;
			dbgprintf("ES:ExportTitleDone():%d\n", ret );
		} break;
		case IOCTL_ES_DELETETITLECONTENT:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u64 *Title=(u64*)malloca( 8, 0x40);
			memcpy32( Title, (u8*)(v[0].data), 8 );

			_sprintf( path, "/title/%08x/%08x/content", (u32)(*Title>>32), (u32)(*Title) );
			ret = ISFS_Delete( path );
			if( ret >= 0 )
				ISFS_CreateDir( path, 0, 3, 3, 3 );

			free( path );

			dbgprintf("ES:DeleteTicket(%08x-%08x):%d\n", (u32)(*Title>>32), (u32)(*Title), ret );
			free( Title );
		} break;
		case IOCTL_ES_DELETETICKET:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u64 *Title=(u64*)malloca( 8, 0x40);
			memcpy32( Title, (u8*)(v[0].data), 8 );

			_sprintf( path, "/title/%08x/%08x", (u32)(*Title>>32), (u32)(*Title) );

			ret = ISFS_Delete( path );

			free( path );

			dbgprintf("ES:DeleteTicket(%08x-%08x):%d\n", (u32)(*Title>>32), (u32)(*Title), ret );
			free( Title );
		} break;
		case IOCTL_ES_DELETETITLE:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u64 *Title=(u64*)malloca( 8, 0x40);
			memcpy32( Title, (u8*)(v[0].data), 8 );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*Title>>32), (u32)(*Title) );
			ret = ISFS_Delete( path );

			if( ret >= 0 )
			{
				_sprintf( path, "/title/%08x/%08x", (u32)(*Title>>32), (u32)(*Title) );
				ret = ISFS_Delete( path );
			}

			heap_free( 0 ,path );
	
			ret = ES_SUCCESS;
			dbgprintf("ES:DeleteTitle(%08x-%08x):%d\n", (u32)(*Title>>32), (u32)(*Title), ret );
			free( Title );
		} break;
		case IOCTL_ES_GETTITLECONTENTS:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );

			u64 *Title=(u64*)malloca( 8, 0x40);
			memcpy32( Title, (u8*)(v[0].data), 8 );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*Title>>32), (u32)(*Title) );

			u8 *data = NANDLoadFile( path, size );
			if( data != NULL )
			{
				u32 count=0;
				for( i=0; i < *(u16*)(data+0x1DE); ++i )
				{	
					if( (*(u16*)(data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
					{
						if( ES_CheckS1Conent( (u8*)(data+0x1F4+i*0x24) ) == 1 )
						{
							*(u32*)(v[2].data+0x4*count) = *(u32*)(data+0x1E4+i*0x24);
							count++;
						}
					} else {
						_sprintf( path, "/title/%08x/%08x/content/%08x.app", *(u32*)(data+0x18C), *(u32*)(data+0x190), *(u32*)(data+0x1E4+i*0x24) );
						s32 fd = IOS_Open( path, 1 );
						if( fd >= 0 )
						{
							*(u32*)(v[2].data+0x4*count) = *(u32*)(data+0x1E4+i*0x24);
							count++;
							IOS_Close( fd );
						}
					}
				}

				free( data );

				ret = ES_SUCCESS;

			} else {
				ret = *size;
			}

			free( size );
			free( path );


			dbgprintf("ES:GetTitleContentsOnCard( %08x-%08x ):%d\n", (u32)(*Title>>32), (u32)(*Title), ret );
			free( Title );
		} break;
		case IOCTL_ES_GETTITLECONTENTSCNT:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );
			u64 *Title=(u64*)malloca( 8, 0x40);
			memcpy32( Title, (u8*)(v[0].data), 8 );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*Title>>32), (u32)(*Title) );

			u8 *data = NANDLoadFile( path, size );
			if( data != NULL )
			{
				*(u32*)(v[1].data) = 0;
			
				for( i=0; i < *(u16*)(data+0x1DE); ++i )
				{	
					if( (*(u16*)(data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
					{
						if( ES_CheckS1Conent( (u8*)(data+0x1F4+i*0x24) ) == 1 )
							(*(u32*)(v[1].data))++;
					} else {
						_sprintf( path, "/title/%08x/%08x/content/%08x.app", *(u32*)(data+0x18C), *(u32*)(data+0x190), *(u32*)(data+0x1E4+i*0x24) );
						s32 fd = IOS_Open( path, 1 );
						if( fd >= 0 )
						{
							(*(u32*)(v[1].data))++;
							IOS_Close( fd );
						}
					}
				}
				
				ret = ES_SUCCESS;
				free( data );

			} else {
				ret = *size;
			}

			free( size );
			free( path );

			dbgprintf("ES:GetTitleContentCount( %08x-%08x, %d):%d\n", (u32)(*Title>>32), (u32)(*Title), *(u32*)(v[1].data), ret );
			free( Title );
		} break;
		case IOCTL_ES_GETTMDVIEWS:
		{
			u64 *Title = malloca( 8, 0x40 );
			memcpy32( Title, (u8*)(v[0].data), 8 );

			ret = ES_GetTMDView( Title, (u8*)(v[2].data) );

			dbgprintf("ES:GetTMDView( %08x-%08x ):%d\n", (u32)(*Title>>32), (u32)*Title, ret );
			free( Title );
		} break;
		case IOCTL_ES_DIGETTICKETVIEW:
		{
			if( v[0].len )
			{
				hexdump( (u8*)(v[0].data), v[1].len );
				while(1);
			}

			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );

			_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(TitleID>>32), (u32)TitleID );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = ES_DIGetTicketView( &TitleID, (u8*)(v[1].data) );
			} else  {

				iES_GetTicketView( data, (u8*)(v[1].data) );
				
				free( data );
				ret = ES_SUCCESS;
			}

			free( path );
			free( size );

			dbgprintf("ES:GetDITicketViews( %08x-%08x ):%d\n", (u32)(TitleID>>32), (u32)TitleID, ret );
		} break;
		case IOCTL_ES_GETVIEWS:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );
			u64 *Title = malloca( 8, 0x40 );

			memcpy32( Title, (u8*)(v[0].data), 8 );

			_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(*Title>>32), (u32)*Title );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = *size;
			} else  {

				for( i=0; i < *(u32*)(v[1].data); ++i )
					iES_GetTicketView( data + i * 0x2A4, (u8*)(v[2].data) + i * 0xD8 );
				
				free( data );
				ret = ES_SUCCESS;
			}

			free( path );
			free( size );

			dbgprintf("ES:GetTicketViews( %08x-%08x ):%d\n", (u32)(*Title>>32), (u32)(*Title), ret );
			free( Title );
		} break;
		case IOCTL_ES_GETTMDVIEWSIZE:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );
			u64 *Title = malloca( 8, 0x40 );
			memcpy32( Title, (u8*)(v[0].data), 8 );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*Title>>32), (u32)(*Title) );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = *size;
			} else {
				*(u32*)(v[1].data) = *(u16*)(data+0x1DE)*16+0x5C;

				free( data );
				ret = ES_SUCCESS;
			}

			free( size );
			free( path );

			dbgprintf("ES:GetTMDViewSize( %08x-%08x, %d ):%d\n", (u32)(*Title>>32), (u32)(*Title), *(u32*)(v[1].data), ret );
			free( Title );
		} break;
		case IOCTL_ES_DIGETTMDVIEW:
		{
			u8 *data = (u8*)malloca( v[0].len, 0x40 );
			memcpy32( data, (u8*)(v[0].data), v[0].len );

			iES_GetTMDView( data, (u8*)(v[2].data) );

			free( data );
			ret = ES_SUCCESS;
			dbgprintf("ES:DIGetTMDView():%d\n", ret );
		} break;
		case IOCTL_ES_DIGETTMDVIEWSIZE:
		{
			u8 *data = (u8*)malloca( v[0].len, 0x40 );
			memcpy32( data, (u8*)(v[0].data), v[0].len );

			*(u32*)(v[1].data) = *(u16*)(data+0x1DE)*16+0x5C;

			free( data );

			ret = ES_SUCCESS;
			dbgprintf("ES:DIGetTMDViewSize( %d ):%d\n", *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_CLOSECONTENT:
		{
			IOS_Close( *(u32*)(v[0].data) );
			ret = ES_SUCCESS;
			if( ret < 0 )			
				dbgprintf("ES:CloseContent(%d):%d\n", *(u32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_SEEKCONTENT:
		{
			ret = IOS_Seek( *(u32*)(v[0].data), *(u32*)(v[1].data), *(u32*)(v[2].data) );
			if( ret < 0 )
				dbgprintf("ES:SeekContent( %d, %d, %d ):%d\n", *(u32*)(v[0].data), *(u32*)(v[1].data), *(u32*)(v[2].data), ret );
		} break;
		case IOCTL_ES_READCONTENT:
		{
			ret = IOS_Read( *(u32*)(v[0].data), (u8*)(v[1].data), v[1].len );
			if( ret < 0 )			
				dbgprintf("ES:ReadContent( %d, %p, %d ):%d\n", *(u32*)(v[0].data), v[1].data, v[1].len, ret );
		} break;
		case IOCTL_ES_OPENCONTENT:
		{
			ret = ES_OpenContent( TitleID, *(u32*)(v[0].data) );
			//if( ret < 0 )
				dbgprintf("ES:OpenContent(%d):%d\n", *(u32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_OPENTITLECONTENT:
		{
			u64 *Title = malloca( 8, 0x40 );
			memcpy32( Title, (u8*)(v[0].data), 8 );

			ret = ES_OpenContent( *Title, *(u32*)(v[2].data) );

		//	if( ret < 0 )
				dbgprintf("ES:OpenTitleContent( %08x-%08x, %d):%d\n", (u32)(*Title>>32), (u32)(*Title), *(u32*)(v[2].data), ret );

			free( Title );
		} break;
		case IOCTL_ES_GETTITLEDIR:
		{
			char *path = (char*)malloca( 0x30, 0x40 );
			memset32( path, 0, 0x30 );

			_sprintf( path, "/title/%08x/%08x/data", (u32)((*(u64*)(v[0].data))>>32), (u32)(*(u64*)(v[0].data)) );

			memcpy32( (u8*)(v[1].data), path, 32 );

			free( path );

			ret = ES_SUCCESS;
			dbgprintf("ES:GetTitleDataDir(%s):%d\n", v[1].data, ret );
		} break;
		case IOCTL_ES_LAUNCH:
		{
			dbgprintf("ES:LaunchTitle( %08x-%08x )\n", (u32)((*(u64*)(v[0].data))>>32), (u32)(*(u64*)(v[0].data)) );

			ret = ES_LaunchTitle( (u64*)(v[0].data), (u8*)(v[1].data) );
		} break;
		case IOCTL_ES_SETUID:
		{
			char *path = (char*)malloca( 0x40, 32 );
			u32 *size = (u32*)malloca( 4, 32 );

			TitleID = *(u64*)(v[0].data);

			u16 UID = 0;
			ret = ES_GetUID( &TitleID, &UID );
			if( ret >= 0 )
			{
				ret = SetUID( 0xF, UID );
				dbgprintf("ES:SetUID( 0xF, %04X ):%d\n", UID, ret );
				if( ret >= 0 )
				{
					_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)TitleID );
					u8 *TMD_Data = NANDLoadFile( path, size );
					if( TMD_Data == NULL )
					{
						ret = *size;
					} else {
						ret = _cc_ahbMemFlush( 0xF, *(u16*)(TMD_Data+0x198) );
						dbgprintf("_cc_ahbMemFlush( %d, %04X ):%d\n", 0xF, *(u16*)(TMD_Data+0x198), ret );
					}

					free( TMD_Data );
				}
			}

			free( size );
			free( path );

			dbgprintf("ES:SetUID(%08x-%08x):%d\n", (u32)(TitleID>>32), (u32)TitleID, ret );
		} break;
		case IOCTL_ES_GETTITLEID:
		{
			*(u64*)(v[0].data) = TitleID;

			ret = ES_SUCCESS;
			dbgprintf("ES:GetTitleID(%08x-%08x):%d\n", (u32)(*(u64*)(v[0].data)>>32), (u32)*(u64*)(v[0].data), ret );
		} break;
		case IOCTL_ES_GETOWNEDTITLECNT:
		{
			ret = ES_GetNumOwnedTitles( (u32*)(v[0].data) );
			dbgprintf("ES:GetOwnedTitleCount(%d):%d\n", *(u32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_GETTITLECNT:
		{
			ret = ES_GetNumTitles( (u32*)(v[0].data) );
			dbgprintf("ES:GetTitleCount(%d):%d\n", *(u32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_GETOWNEDTITLES:
		{
			ret = ES_GetOwnedTitles( (u64*)(v[1].data) );
			dbgprintf("ES:GetOwnedTitles():%d\n", ret );
		} break;
		case IOCTL_ES_GETTITLES:
		{
			ret = ES_GetTitles( (u64*)(v[1].data) );
			dbgprintf("ES:GetTitles():%d\n", ret );
		} break;
		/*
			Returns 0 views if title is not installed but always ES_SUCCESS as return
		*/
		case IOCTL_ES_GETVIEWCNT:
		{
			u64 *Title = malloca( 8, 0x40 );
			memcpy32( Title, (u8*)(v[0].data), 8 );
			char *path = (char*)malloca( 0x40, 32 );

			_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(*Title>>32), (u32)(*Title) );

			s32 fd = IOS_Open( path, 1 );
			if( fd < 0 )
			{
				*(u32*)(v[1].data) = 0;
			} else {
				u32 size = IOS_Seek( fd, 0, SEEK_END );
				*(u32*)(v[1].data) = size / 0x2A4;
				IOS_Close( fd );
			}

			ret = ES_SUCCESS;

			dbgprintf("ES:GetTicketViewCount( %08x-%08x, %d):%d\n", (u32)(*Title>>32), (u32)(*Title), *(u32*)(v[1].data), ret );

			free( Title );
			free( path );

		} break;
		case IOCTL_ES_GETSTOREDTMDSIZE:
		{
			char *path = (char*)malloca( 0x40, 32 );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)((*(u64*)(v[0].data))>>32), (u32)((*(u64*)(v[0].data))) );

			s32 fd = IOS_Open( path, 1 );
			if( fd < 0 )
			{
				ret = fd;
				*(u32*)(v[1].data) = 0;

			} else {

				fstats *status = (fstats*)malloc( sizeof(fstats) );

				ret = ISFS_GetFileStats( fd, status );
				if( ret < 0 )
				{
					*(u32*)(v[1].data) = 0;
				} else {
					*(u32*)(v[1].data) = status->Size;
				}

				IOS_Close( fd );
				free( status );
			}

			free( path );
			dbgprintf("ES:GetStoredTMDSize( %08x-%08x, %d ):%d\n", (u32)((*(u64*)(v[0].data))>>32), (u32)((*(u64*)(v[0].data))), *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_GETSTOREDTMD:
		{
			char *path = (char*)malloca( 0x40, 32 );
			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)((*(u64*)(v[0].data))>>32), (u32)((*(u64*)(v[0].data))) );

			s32 fd = IOS_Open( path, 1 );
			if( fd < 0 )
			{
				ret = fd;

			} else {

				ret = IOS_Read( fd, (u8*)(v[2].data), v[2].len );
				if( ret == v[2].len )
					ret = ES_SUCCESS;

				IOS_Close( fd );
			}

			free( path );

			dbgprintf("ES:GetStoredTMD(%08x-%08x):%d\n", (u32)((*(u64*)(v[0].data))>>32), (u32)((*(u64*)(v[0].data))), ret );
		} break;
		case 0x3B:
		{
			/*
				data:138f0300 len:2560(0xA00)
				data:00000000 len:0(0x0)
				data:138f0fc0 len:216(0xD8)
				data:138f0d40 len:520(0x208)
				data:138f1720 len:4(0x4)
				data:138f0f80 len:20(0x14)
				ES:IOS_Ioctlv( 154 0x3b 4 2 0x138f17f0 )
			*/
			ret = ES_FATAL;
			dbgprintf("ES:DIVerfiyTikView():%d\n", ret );
		} break;
		case IOCTL_ES_DIVERIFY:
		{
			ret = ES_SUCCESS;

			if( (u32*)(v[4].data) == NULL )		// key
			{
				dbgprintf("key ptr == NULL\n");
				ret = ES_FATAL;
			} else if( v[4].len != 4 ) {
				dbgprintf("key len invalid, %d != 4\n", v[4].len );
				ret = ES_FATAL;
			}

			if( (u8*)(v[3].data) == NULL )		// TMD
			{
				dbgprintf("TMD ptr == NULL\n");
				ret = ES_FATAL;
			} else if( ((*(u16*)(v[3].data+0x1DE))*36+0x1E4) != v[3].len ) {
				dbgprintf("TMD len invalid, %d != %d\n", ((*(u16*)(v[3].data+0x1DE))*36+0x1E4), v[3].len );
				ret = ES_FATAL;
			}

			if( (u8*)(v[2].data) == NULL )		// tik
			{
				dbgprintf("tik ptr == NULL\n");
				ret = ES_FATAL;
			} else if( v[2].len != 0x2A4 ) {
				dbgprintf("tik len invalid, %d != 0x2A4\n", v[2].len );
				ret = ES_FATAL;
			}

			if( (u8*)(v[5].data) == NULL )		//hashes
			{
				dbgprintf("hashes ptr == NULL\n");
				ret = ES_FATAL;
			 }

			if( ret == ES_SUCCESS )
				ret = ES_DIVerify( &TitleID, (u32*)(v[4].data), (u8*)(v[3].data), v[3].len, (u8*)(v[2].data), (u8*)(v[5].data) );

			dbgprintf("ES:DIVerfiy():%d\n", ret );
		} break;
		case IOCTL_ES_GETBOOT2VERSION:
		{
			*(u32*)(v[0].data) = 5;
			ret = ES_SUCCESS;
			dbgprintf("ES:GetBoot2Version(5):%d\n", ret );
		} break;
		case 0x45:
		{
			ret = -1017;
			dbgprintf("ES:Error_003():%d\n", ret );
		} break;
		case IOCTL_ES_IMPORTBOOT:
		{
			ret = ES_SUCCESS;
			dbgprintf("ES:ImportBoot():%d\n", ret );
		} break;
		default:
			for( i=0; i<InCount+OutCount; ++i)
			{
				dbgprintf("data:%p len:%d(0x%X)\n", v[i].data, v[i].len, v[i].len );
				hexdump( v[i].data, v[i].len );
			}
			dbgprintf("ES:IOS_Ioctlv( %d 0x%x %d %d 0x%p )\n", msg->fd, msg->ioctlv.command, msg->ioctlv.argc_in, msg->ioctlv.argc_io, msg->ioctlv.argv);
			while(1);
		break;
	}

	mqueue_ack( (void *)msg, ret);

}
s32 RegisterDevices( void )
{
	queueid = mqueue_create( queuespace, 2 );

	s32 ret = device_register("/dev/es", queueid );
#ifdef DEBUG
	dbgprintf("ES:DeviceRegister(\"/dev/es\"):%d QueueID:%d\n", ret, queueid );
#endif
	if( ret < 0 )
		return ret;
	
	ret = device_register("/dev/sdio", queueid );
#ifdef DEBUG
	dbgprintf("ES:DeviceRegister(\"/dev/sdio\"):%d QueueID:%d\n", ret, queueid );
#endif
	if( ret < 0 )
		return ret;

	return queueid;
}


int _main( int argc, char *argv[] )
{
	thread_set_priority( 0, 0x79 );
	thread_set_priority( 0, 0x50 );
	thread_set_priority( 0, 0x79 );

#ifdef DEBUG
	dbgprintf("$IOSVersion: ES: %s %s 64M DEBUG$\n", __DATE__, __TIME__ );
#else
	dbgprintf("$IOSVersion: ES: %s %s 64M Release$\n", __DATE__, __TIME__ );
#endif

	KernelVersion = *(vu32*)0x00003140;

	int queueid=0;
	s32 ret=0;
	struct ipcmessage *message=NULL;

	dbgprintf("ES:Heap Init...");
	heapid = heap_create( heap, 0x100 );
	queuespace = heap_alloc( heapid, 0x20);
	dbgprintf("ok\n");

	queueid = RegisterDevices();
	if( queueid < 0 )
	{
		ThreadCancel( 0, 0x77 );
	}

	s32 Timer = timer_create( 0, 0, queueid, 0xDEADDEAD );
	dbgprintf("ES:Timer:%d\n", Timer );

	u32 pid = GetPID();
	dbgprintf("ES:GetPID():%d\n", pid );
	ret = SetUID( pid, 0 );
	dbgprintf("ES:SetUID():%d\n", ret );
	ret = _cc_ahbMemFlush( pid, 0 );
	dbgprintf("ES:_cc_ahbMemFlush():%d\n", ret );

	u32 Flag1;
	u16	Flag2;
	GetFlags( &Flag1, &Flag2 );
	dbgprintf("ES:Flag1:%d Flag2:%d\n", Flag1, Flag2 );

	u32 version = GetKernelVersion();
	dbgprintf("ES:KernelVersion:%08X, %d\n", version, (version<<8)>>0x18 );

	ret = ISFS_Init();
	dbgprintf("ES:ISFS_Init():%d\n", ret );

	ES_BootSystem( &TitleID, &KernelVersion );

	dbgprintf("ES:looping!\n");

	SDStatus = malloca( sizeof(u32), 0x40 );
	*SDStatus = 0x00000002;

	HCR = malloca( sizeof(u32)*0x30, 0x40 );
	memset32( HCR, 0, sizeof(u32)*0x30 );

	//WiiShop always swimming mario
	//if( *(u32*)0x0001CCC0 == 0x4082007C )
	//	*(u32*)0x0001CCC0 = 0x38000063;

	//Region free 4.2EUR
	//if( TitleID == 0x0000000100000002LL )
	//{
	//	*(u32*)0x0137DC90 = 0x4800001C;
	//	*(u32*)0x0137E4E4 = 0x60000000;
	//}

	while (1)
	{
		ret = mqueue_recv( queueid, (void *)&message, 0);
		if( ret != 0 )
		{
			dbgprintf("ES:mqueue_recv(%d) FAILED:%d\n", queueid, ret);
			continue;
		}
	
		//dbgprintf("ES:mqueue_recv(%d):%d cmd:%d ioctlv:\"%X\"\n", queueid, ret, message->command, message->ioctlv.command );
				
		switch( message->command )
		{
			case IOS_OPEN:
			{
				//dbgprintf("ES:mqueue_recv(%d):%d cmd:%d device:\"%s\"\n", queueid, ret, message->command, message->open.device );
				// Is it our device?
				if( strncmp( message->open.device, "/dev/es", 7 ) == 0 )
				{
					ret = ES_FD;
				} else if( strncmp( message->open.device, "/dev/sdio", 9 ) == 0) {
					ret = SD_FD;
				} else  {
					ret = FS_ENOENT;
				}
				
				mqueue_ack( (void *)message, ret );
				
			} break;
			
			case IOS_CLOSE:
			{
#ifdef DEBUG
				dbgprintf("ES:IOS_Close(%d)\n", message->fd );
#endif
				if( message->fd == ES_FD || message->fd == SD_FD  )
				{
					mqueue_ack( (void *)message, ES_SUCCESS);
					break;
				} else 
					mqueue_ack( (void *)message, FS_EINVAL );

			} break;

			case IOS_READ:
			case IOS_WRITE:
			case IOS_SEEK:
			{
				dbgprintf("ES:Error Read|Write|Seek called!\n");
				mqueue_ack( (void *)message, FS_EINVAL );
			} break;
			case IOS_IOCTL:
			{
				if( message->fd == SD_FD ) 
					SD_Ioctl( message );
				else
					mqueue_ack( (void *)message, FS_EINVAL );

			} break;

			case IOS_IOCTLV:
				if( message->fd == ES_FD )
					ES_Ioctlv( message );
				else if( message->fd == SD_FD ) 
					SD_Ioctlv( message );
				else
					mqueue_ack( (void *)message, FS_EINVAL );
			break;
			
			default:
				dbgprintf("ES:unimplemented/invalid msg: %08x argv[0]:%08x\n", message->command, message->args[0] );
				mqueue_ack( (void *)message, FS_EINVAL );
				while(1);
		}
	}

	return 0;
}
