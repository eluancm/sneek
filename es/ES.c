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
#include "ES.h"
#include "ESP.h"
#include "DI.h"

extern u32 *KeyID;

char *path		= (char*)NULL;
u32 *size		= (u32*)NULL;
u64 *iTitleID	= (u64*)NULL;


static u64 TitleID ALIGNED(32);
static u32 KernelVersion ALIGNED(32);
static u32 SkipContent ALIGNED(32);

extern u32 TitleVersion;

TitleMetaData *iTMD = (TitleMetaData *)NULL;			//used for information during title import
static u8 *iTIK		= (u8 *)NULL;						//used for information during title import

u32 ES_Init( u8 *MessageHeap )
{
	KernelVersion = *(vu32*)0x00003140;
	dbgprintf("ES:KernelVersion:%08X, %d\n", KernelVersion, (KernelVersion<<8)>>0x18 );

	u32 MessageQueue = mqueue_create( MessageHeap, 1 );

	device_register( "/dev/es", MessageQueue );

	u32 pid = GetPID();
	u32 ret = SetUID( pid, 0 );
		ret = _cc_ahbMemFlush( pid, 0 );

	//u32 Flag1;
	//u16 Flag2;
	//GetFlags( &Flag1, &Flag2 );

	u32 version = GetKernelVersion();
	dbgprintf("ES:KernelVersion:%08X, %d\n", version, (version<<8)>>0x18 );
	
	ret = ISFS_Init();

	dbgprintf("ES:ISFS_Init():%d\n", ret );
	
//Used in Ioctlvs
	path		= (char*)malloca(		0x40,  32 );
	size		= (u32*) malloca( sizeof(u32), 32 );
	iTitleID	= (u64*) malloca( sizeof(u64), 32 );

	ES_BootSystem( &TitleID, &KernelVersion );

	dbgprintf("ES:TitleID:%08x-%08x version:%d\n", (u32)((TitleID)>>32), (u32)(TitleID), TitleVersion );

	SMenuInit( TitleID, TitleVersion );
	
	return MessageQueue;
}
u64 ES_GetTitleID( void )
{
	return TitleID;
}

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
		iTIK = (u8*)NULL;
	}
}
void ES_Ioctlv( struct ipcmessage *msg )
{
	u32 InCount		= msg->ioctlv.argc_in;
	u32 OutCount	= msg->ioctlv.argc_io;
	vector *v		= (vector*)(msg->ioctlv.argv);
	s32 ret			= ES_FATAL;
	u32 i;

	//dbgprintf("ES:IOS_Ioctlv( %d 0x%x %d %d 0x%p )\n", msg->fd, msg->ioctlv.command, msg->ioctlv.argc_in, msg->ioctlv.argc_io, msg->ioctlv.argv);
	////	
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
			ret = ESP_Sign( &TitleID, (u8*)(v[0].data), v[0].len, (u8*)(v[1].data), (u8*)(v[2].data) );
			dbgprintf("ES:Sign():%d\n", ret );
		} break;
		case IOCTL_ES_DIGETSTOREDTMD:
		{
			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)(TitleID) );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = *size;
			} else {
				memcpy( (u8*)(v[1].data), data, *size );
				
				ret = ES_SUCCESS;
				free( data );
			}

			dbgprintf("ES:DIGetStoredTMD( %08x-%08x ):%d\n", (u32)(TitleID>>32), (u32)(TitleID), ret );
		} break;
		case IOCTL_ES_DIGETSTOREDTMDSIZE:
		{
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
				} else
					*(u32*)(v[0].data) = status->Size;

				free( status );
			}

			dbgprintf("ES:DIGetStoredTMDSize( %08x-%08x ):%d\n", (u32)(TitleID>>32), (u32)(TitleID), ret );
			
		} break;
		case IOCTL_ES_GETSHAREDCONTENTS:
		{
			_sprintf( path, "/shared1/content.map" );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = ES_FATAL;
			} else {

				for( i=0; i < *(u32*)(v[0].data); ++i )
					memcpy( (u8*)(v[1].data+i*20), data+i*0x1C+8, 20 );

				free( data );
				ret = ES_SUCCESS;
			}

			dbgprintf("ES:ES_GetSharedContents():%d\n", ret );
		} break;
		case IOCTL_ES_GETSHAREDCONTENTCNT:
		{
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

			dbgprintf("ES:ES_GetSharedContentCount(%d):%d\n", *(vu32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_GETTMDCONTENTCNT:
		{
			*(u32*)(v[1].data) = 0;
			TitleMetaData *tTMD = (TitleMetaData*)(v[1].data);
		
			for( i=0; i < tTMD->ContentCount; ++i )
			{	
				if( tTMD->Contents[i].Type & CONTENT_SHARED )
				{
					if( ES_CheckSharedContent( tTMD->Contents[i].SHA1 ) == 1 )
						(*(u32*)(v[1].data))++;
				} else {
					_sprintf( path, "/title/%08x/%08x/content/%08x.app", *(u32*)(v[0].data+0x18C), *(u32*)(v[0].data+0x190), tTMD->Contents[i].ID );
					s32 fd = IOS_Open( path, 1 );
					if( fd >= 0 )
					{
						(*(u32*)(v[1].data))++;
						IOS_Close( fd );
					}
				}
			}

			ret = ES_SUCCESS;
			dbgprintf("ES:GetTmdContentsOnCardCount(%d):%d\n", *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_GETTMDCONTENTS:
		{
			u32 count=0;

			for( i=0; i < *(u16*)(v[0].data+0x1DE); ++i )
			{	
				if( (*(u16*)(v[0].data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
				{
					if( ES_CheckSharedContent( (u8*)(v[0].data+0x1F4+i*0x24) ) == 1 )
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

			ret = ES_SUCCESS;
			dbgprintf("ES:ListTmdContentsOnCard():%d\n", ret );
		} break;
		case IOCTL_ES_GETDEVICECERT:
		{
			_sprintf( path, "/sys/device.cert" );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				GetDeviceCert( (void*)(v[0].data) );
			} else {
				memcpy( (u8*)(v[0].data), data, 0x180 );
				free( data );
			}

			ret = ES_SUCCESS;
			dbgprintf("ES:GetDeviceCert():%d\n", ret );			
		} break;
		case IOCTL_ES_GETDEVICEID:
		{
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
				free( data );
			}

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
			ret = ESP_AddTitleFinish( iTMD );

			//Get TMD for the CID names and delete!
			_sprintf( path, "/tmp/title.tmd" );
			ISFS_Delete( path );

			for( i=0; i < iTMD->ContentCount; ++i )
			{
				_sprintf( path, "/tmp/%08x", iTMD->Contents[i].ID );
				ISFS_Delete( path );
				_sprintf( path, "/tmp/%08x.app", iTMD->Contents[i].ID );
				ISFS_Delete( path );
			}

			////TODO: Delete contents from old versions of this title
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
			_sprintf( path, "/tmp/title.tmd" );
			ISFS_Delete( path );
			
			for( i=0; i < iTMD->ContentCount; ++i )
			{
				_sprintf( path, "/tmp/%08x", iTMD->Contents[i].ID );
				ISFS_Delete( path );
				_sprintf( path, "/tmp/%08x.app", iTMD->Contents[i].ID );
				ISFS_Delete( path );
			}

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

			if( iTMD == NULL )
				ret = ES_FATAL;
			else {
				//load Ticket to forge the decryption key
				_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(iTMD->TitleID>>32), (u32)(iTMD->TitleID) );

				iTIK = NANDLoadFile( path, size );
				if( iTIK == NULL )
				{
					iCleanUpTikTMD();
					ret = ES_ETIKTMD;

				} else {

					ret = ES_CreateKey( iTIK );
					if( ret >= 0 )
					{
						ret = ESP_AddContentFinish( *(vu32*)(v[0].data), iTIK, iTMD );
						DestroyKey( *KeyID );
					}

					free( iTIK );
				}
			}

			dbgprintf("ES:AddContentFinish():%d\n", ret );
		} break;
		case IOCTL_ES_ADDCONTENTDATA:
		{
			if( SkipContent )
			{
				ret = ES_SUCCESS;
				dbgprintf("ES:AddContentData(<fast>):%d\n", ret );
				break;
			}

			if( iTMD == NULL )
				ret = ES_FATAL;
			else {
				ret = ESP_AddContentData( *(s32*)(v[0].data), (u8*)(v[1].data), v[1].len );
			}

			//if( ret < 0 )
				dbgprintf("ES:AddContentData( %d, 0x%p, %d ):%d\n", *(s32*)(v[0].data), (u8*)(v[1].data), v[1].len, ret );
		} break;
		case IOCTL_ES_ADDCONTENTSTART:
		{
			SkipContent=0;

			if( iTMD == NULL )
				ret = ES_FATAL;
			else {
				//check if shared content and if it is already installed so we can skip this one
				for( i=0; i < iTMD->ContentCount; ++i )
				{
					if( iTMD->Contents[i].ID == *(u32*)(v[1].data) )
					{
						if( iTMD->Contents[i].Type & CONTENT_SHARED )
						{
							if( ES_CheckSharedContent( iTMD->Contents[i].SHA1 ) == 1 )
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
			//Copy TMD to internal buffer for later use
			iTMD = (TitleMetaData*)malloca( v[0].len, 32 );
			memcpy( iTMD, (u8*)(v[0].data), v[0].len );

			_sprintf( path, "/tmp/title.tmd" );

			ES_TitleCreatePath( iTMD->TitleID );

			ret = ISFS_CreateFile( path, 0, 3, 3, 3 );
			if( ret < 0 )
			{
				dbgprintf("ISFS_CreateFile(\"%s\"):%d\n", path, ret );
			} else {

				s32 fd = IOS_Open( path, ISFS_OPEN_WRITE );
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
				}
			}

			if( ret == ES_SUCCESS )
			{
				//Add new TitleUID to uid.sys
				u16 UID = 0;
				ESP_GetUID( &(iTMD->TitleID), &UID );
			}

			dbgprintf("ES:AddTitleStart():%d\n", ret );
		} break;
		case IOCTL_ES_ADDTICKET:
		{
			//Copy ticket to local buffer
			Ticket *ticket = (Ticket*)malloca( v[0].len, 32 );
			memcpy( ticket, (u8*)(v[0].data), v[0].len );
			
			_sprintf( path, "/tmp/%08x.tik", (u32)(ticket->TitleID) );

			if( ticket->ConsoleID )
				doTicketMagic( ticket );

			ES_TitleCreatePath( ticket->TitleID );

			ret = ISFS_CreateFile( path, 0, 3, 3, 3 );
			if( ret < 0 )
			{
				dbgprintf("ES:ISFS_CreateFile(\"%s\"):%d\n", path, ret );
			} else {

				s32 fd = IOS_Open( path, ISFS_OPEN_WRITE );
				if( fd < 0 )
				{
					dbgprintf("ES:IOS_Open(\"%s\"):%d\n", path, fd );
					ret = fd;
				} else {

					ret = IOS_Write( fd, ticket, v[0].len );
					if( ret < 0 || ret != v[0].len )
					{
						dbgprintf("ES:IOS_Write( %d, %p, %d):%d\n", fd, ticket, v[0].len, ret );

					} else {

						IOS_Close( fd );

						char *dstpath = (char*)malloca( 0x40, 32 );

						_sprintf( path, "/tmp/%08x.tik", (u32)(ticket->TitleID) );
						_sprintf( dstpath, "/ticket/%08x/%08x.tik", (u32)(ticket->TitleID>>32), (u32)(ticket->TitleID) );

						//this function moves the file, overwriting the target
						ret = ISFS_Rename( path, dstpath );
						if( ret < 0 )
							dbgprintf("ES:ISFS_Rename( \"%s\", \"%s\" ):%d\n", path, dstpath, ret );

						free( dstpath );
					}
				}
			}

			dbgprintf("ES:AddTicket(%08x-%08x):%d\n", (u32)(ticket->TitleID>>32), (u32)(ticket->TitleID), ret );
			free( ticket );
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
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/title/%08x/%08x/content", (u32)(*iTitleID>>32), (u32)(*iTitleID) );
			ret = ISFS_Delete( path );
			if( ret >= 0 )
				ISFS_CreateDir( path, 0, 3, 3, 3 );

			dbgprintf("ES:DeleteTitleContent(%08x-%08x):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );
		} break;
		case IOCTL_ES_DELETETICKET:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

			ret = ISFS_Delete( path );

			dbgprintf("ES:DeleteTicket(%08x-%08x):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );
		} break;
		case IOCTL_ES_DELETETITLE:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/title/%08x/%08x", (u32)(*iTitleID>>32), (u32)(*iTitleID) );
			ret = ISFS_Delete( path );

			dbgprintf("ES:DeleteTitle(%08x-%08x):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );
		} break;
		case IOCTL_ES_GETTITLECONTENTS:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

			TitleMetaData *tTMD = (TitleMetaData*)NANDLoadFile( path, size );
			if( tTMD != NULL )
			{
				u32 count=0;
				for( i=0; i < tTMD->ContentCount; ++i )
				{	
					if( tTMD->Contents[i].Type & CONTENT_SHARED )
					{
						if( ES_CheckSharedContent( tTMD->Contents[i].SHA1 ) == 1 )
						{
							*(u32*)(v[2].data+0x4*count) = tTMD->Contents[i].ID;
							count++;
						}
					} else {
						_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)(tTMD->TitleID>>32), (u32)(tTMD->TitleID), tTMD->Contents[i].ID );
						s32 fd = IOS_Open( path, 1 );
						if( fd >= 0 )
						{
							*(u32*)(v[2].data+0x4*count) = tTMD->Contents[i].ID;
							count++;
							IOS_Close( fd );
						}
					}
				}

				free( tTMD );

				ret = ES_SUCCESS;

			} else {
				ret = *size;
			}

			dbgprintf("ES:GetTitleContentsOnCard( %08x-%08x ):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );
		} break;
		case IOCTL_ES_GETTITLECONTENTSCNT:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

			TitleMetaData *TMD = (TitleMetaData *)NANDLoadFile( path, size );
			if( TMD != NULL )
			{
				*(u32*)(v[1].data) = 0;
			
				for( i=0; i < TMD->ContentCount; ++i )
				{	
					if( TMD->Contents[i].Type & CONTENT_SHARED )
					{
						if( ES_CheckSharedContent( TMD->Contents[i].SHA1 ) == 1 )
							(*(u32*)(v[1].data))++;
					} else {
						_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)((TMD->TitleID)>>32), (u32)(TMD->TitleID), TMD->Contents[i].ID );
						s32 fd = IOS_Open( path, 1 );
						if( fd >= 0 )
						{
							(*(u32*)(v[1].data))++;
							IOS_Close( fd );
						}
					}
				}
				
				ret = ES_SUCCESS;
				free( TMD );

			} else {
				ret = *size;
			}

			dbgprintf("ES:GetTitleContentCount( %08x-%08x, %d):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_GETTMDVIEWS:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			ret = ESP_GetTMDView( iTitleID, (u8*)(v[2].data) );

			dbgprintf("ES:GetTMDView( %08x-%08x ):%d\n", (u32)(*iTitleID>>32), (u32)*iTitleID, ret );
		} break;
		case IOCTL_ES_DIGETTICKETVIEW:
		{
			if( v[0].len )
			{
				hexdump( (u8*)(v[0].data), v[1].len );
				while(1);
			}

			_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(TitleID>>32), (u32)TitleID );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = ESP_DIGetTicketView( &TitleID, (u8*)(v[1].data) );
			} else  {

				iES_GetTicketView( data, (u8*)(v[1].data) );
				
				free( data );
				ret = ES_SUCCESS;
			}

			dbgprintf("ES:GetDITicketViews( %08x-%08x ):%d\n", (u32)(TitleID>>32), (u32)TitleID, ret );
		} break;
		case IOCTL_ES_GETVIEWS:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(*iTitleID>>32), (u32)*iTitleID );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = *size;
			} else  {

				for( i=0; i < *(u32*)(v[1].data); ++i )
					iES_GetTicketView( data + i * TICKET_SIZE, (u8*)(v[2].data) + i * 0xD8 );
				
				free( data );
				ret = ES_SUCCESS;
			}

			dbgprintf("ES:GetTicketViews( %08x-%08x ):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );
		} break;
		case IOCTL_ES_GETTMDVIEWSIZE:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

			TitleMetaData *TMD = (TitleMetaData *)NANDLoadFile( path, size );
			if( TMD == NULL )
			{
				ret = *size;
			} else {
				*(u32*)(v[1].data) = TMD->ContentCount*16+0x5C;

				free( TMD );
				ret = ES_SUCCESS;
			}

			dbgprintf("ES:GetTMDViewSize( %08x-%08x, %d ):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_DIGETTMDVIEW:
		{
			TitleMetaData *data = (TitleMetaData*)malloca( v[0].len, 0x40 );
			memcpy( data, (u8*)(v[0].data), v[0].len );

			iES_GetTMDView( data, (u8*)(v[2].data) );

			free( data );

			ret = ES_SUCCESS;
			dbgprintf("ES:DIGetTMDView():%d\n", ret );
		} break;
		case IOCTL_ES_DIGETTMDVIEWSIZE:
		{
			TitleMetaData *TMD = (TitleMetaData*)malloca( v[0].len, 0x40 );
			memcpy( TMD, (u8*)(v[0].data), v[0].len );

			*(u32*)(v[1].data) = TMD->ContentCount*16+0x5C;

			free( TMD );

			ret = ES_SUCCESS;
			dbgprintf("ES:DIGetTMDViewSize( %d ):%d\n", *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_CLOSECONTENT:
		{
			IOS_Close( *(u32*)(v[0].data) );

			ret = ES_SUCCESS;
			dbgprintf("ES:CloseContent(%d):%d\n", *(u32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_SEEKCONTENT:
		{
			ret = IOS_Seek( *(u32*)(v[0].data), *(u32*)(v[1].data), *(u32*)(v[2].data) );
			dbgprintf("ES:SeekContent( %d, %d, %d ):%d\n", *(u32*)(v[0].data), *(u32*)(v[1].data), *(u32*)(v[2].data), ret );
		} break;
		case IOCTL_ES_READCONTENT:
		{
			ret = IOS_Read( *(u32*)(v[0].data), (u8*)(v[1].data), v[1].len );

			//int i;
			//for( i=0; i < v[1].len; i+=4 )
			//{
			//	if( memcmp( (void*)((u8*)(v[1].data)+i), sig_fwrite, sizeof(sig_fwrite) ) == 0 )
			//	{
			//		dbgprintf("ES:[patcher] Found __fwrite pattern:%08X\n",  (u32)((u8*)(v[1].data)+i) | 0x80000000 );
			//		memcpy( (void*)((u8*)(v[1].data)+i), patch_fwrite, sizeof(patch_fwrite) );
			//	}
			//	//if( *(vu32*)((u8*)(v[1].data)+i) == 0x3C608000 )
			//	//{
			//	//	if( ((*(vu32*)((u8*)(v[1].data)+i+4) & 0xFC1FFFFF ) == 0x800300CC) && ((*(vu32*)((u8*)(v[1].data)+i+8) >> 24) == 0x54 ) )
			//	//	{
			//	//		//dbgprintf("ES:[patcher] Found VI pattern:%08X\n", (u32)((u8*)(v[1].data)+i) | 0x80000000 );
			//	//		*(vu32*)0xCC = 1;
			//	//		dbgprintf("ES:VideoMode:%d\n", *(vu32*)0xCC );
			//	//		*(vu32*)((u8*)(v[1].data)+i+4) = 0x5400F0BE | ((*(vu32*)((u8*)(v[1].data)+i+4) & 0x3E00000) >> 5 );
			//	//	}
			//	//}
			//}

			dbgprintf("ES:ReadContent( %d, %p, %d ):%d\n", *(u32*)(v[0].data), v[1].data, v[1].len, ret );
		} break;
		case IOCTL_ES_OPENCONTENT:
		{
			ret = ESP_OpenContent( TitleID, *(u32*)(v[0].data) );
			dbgprintf("ES:OpenContent(%d):%d\n", *(u32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_OPENTITLECONTENT:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			ret = ESP_OpenContent( *iTitleID, *(u32*)(v[2].data) );

			dbgprintf("ES:OpenTitleContent( %08x-%08x, %d):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(v[2].data), ret );
		} break;
		case IOCTL_ES_GETTITLEDIR:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/title/%08x/%08x/data", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

			memcpy( (u8*)(v[1].data), path, 32 );

			ret = ES_SUCCESS;
			dbgprintf("ES:GetTitleDataDir(%s):%d\n", v[1].data, ret );
		} break;
		case IOCTL_ES_LAUNCH:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			dbgprintf("ES:LaunchTitle( %08x-%08x )\n", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

			ret = ES_LaunchTitle( (u64*)(v[0].data), (u8*)(v[1].data) );

			dbgprintf("ES_LaunchTitle Failed with:%d\n", ret );

		} break;
		case IOCTL_ES_SETUID:
		{
			memcpy( &TitleID, (u8*)(v[0].data), sizeof(u64) );

			u16 UID = 0;
			ret = ESP_GetUID( &TitleID, &UID );
			if( ret >= 0 )
			{
				ret = SetUID( 0xF, UID );

				if( ret >= 0 )
				{
					_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)TitleID );
					TitleMetaData *TMD = (TitleMetaData *)NANDLoadFile( path, size );
					if( TMD == NULL )
					{
						ret = *size;
					} else {
						ret = _cc_ahbMemFlush( 0xF, TMD->GroupID );
						if( ret < 0 )
							dbgprintf("_cc_ahbMemFlush( %d, %04X ):%d\n", 0xF, TMD->GroupID, ret );
						free( TMD );
					}
				} else {
					dbgprintf("ES:SetUID( 0xF, %04X ):%d\n", UID, ret );
				}
			}

			dbgprintf("ES:SetUID(%08x-%08x):%d\n", (u32)(TitleID>>32), (u32)TitleID, ret );
		} break;
		case IOCTL_ES_GETTITLEID:
		{
			memcpy( (u8*)(v[0].data), &TitleID, sizeof(u64) );

			ret = ES_SUCCESS;
			dbgprintf("ES:GetTitleID(%08x-%08x):%d\n", (u32)(*(u64*)(v[0].data)>>32), (u32)*(u64*)(v[0].data), ret );
		} break;
		case IOCTL_ES_GETOWNEDTITLECNT:
		{
			ret = ESP_GetNumOwnedTitles( (u32*)(v[0].data) );
			dbgprintf("ES:GetOwnedTitleCount(%d):%d\n", *(u32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_GETTITLECNT:
		{
			ret = ESP_GetNumTitles( (u32*)(v[0].data) );
			dbgprintf("ES:GetTitleCount(%d):%d\n", *(u32*)(v[0].data), ret );
		} break;
		case IOCTL_ES_GETOWNEDTITLES:
		{
			ret = ESP_GetOwnedTitles( (u64*)(v[1].data) );
			dbgprintf("ES:GetOwnedTitles():%d\n", ret );
		} break;
		case IOCTL_ES_GETTITLES:
		{
			ret = ESP_GetTitles( (u64*)(v[1].data) );
			dbgprintf("ES:GetTitles():%d\n", ret );
		} break;
		/*
			Returns 0 views if title is not installed but always ES_SUCCESS as return
		*/
		case IOCTL_ES_GETVIEWCNT:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

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
			dbgprintf("ES:GetTicketViewCount( %08x-%08x, %d):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_GETSTOREDTMDSIZE:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

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

			dbgprintf("ES:GetStoredTMDSize( %08x-%08x, %d ):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_GETSTOREDTMD:
		{
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
				ret = ESP_DIVerify( &TitleID, (u32*)(v[4].data), (TitleMetaData*)(v[3].data), v[3].len, (char*)(v[2].data), (char*)(v[5].data) );

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
			ret = ES_FATAL;
			dbgprintf("ES:KoreanKeyCheck():%d\n", ret );
		} break;
		case IOCTL_ES_IMPORTBOOT:
		{
			ret = ES_SUCCESS;
			dbgprintf("ES:ImportBoot():%d\n", ret );
		} break;
		default:
		{
			for( i=0; i<InCount+OutCount; ++i)
			{
				dbgprintf("data:%p len:%d(0x%X)\n", v[i].data, v[i].len, v[i].len );
				hexdump( (u8*)(v[i].data), v[i].len );
			}
			dbgprintf("ES:IOS_Ioctlv( %d 0x%x %d %d 0x%p )\n", msg->fd, msg->ioctlv.command, msg->ioctlv.argc_in, msg->ioctlv.argc_io, msg->ioctlv.argv);
			ret = ES_FATAL;
		} break;
	}

	mqueue_ack( (void *)msg, ret);
}
