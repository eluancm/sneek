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
#include "font.h"
#include "DI.h"
#include "SDI.h"
#include "SMenu.h"

int verbose = 0;
u32 base_offset=0;
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

extern u16 TitleVersion;
extern u32 *KeyID;
extern u8 *CNTMap;
extern u32 *HCR;
extern u32 *SDStatus;

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

char *path=NULL;
u32 *size=NULL;
u64 *iTitleID=NULL;

void ES_Ioctlv( struct ipcmessage *msg )
{
	u32 InCount		= msg->ioctlv.argc_in;
	u32 OutCount	= msg->ioctlv.argc_io;
	vector *v		= (vector*)(msg->ioctlv.argv);
	s32 ret			= ES_FATAL;
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
		
			for( i=0; i < *(u16*)(v[0].data+0x1DE); ++i )
			{	
				if( (*(u16*)(v[0].data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
				{
					if( ES_CheckSharedContent( (u8*)(v[0].data+0x1F4+i*0x24) ) == 1 )
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
			ret = ES_AddTitleFinish( iTMD );

			//Get TMD for the CID names and delete!
			_sprintf( path, "/tmp/title.tmd" );
			ISFS_Delete( path );

			for( i=0; i < *(u16*)(iTMD+0x1DE); ++i )
			{
				_sprintf( path, "/tmp/%08x", *(u32*)(iTMD+0x1E4+i*0x24) );
				ISFS_Delete( path );
				_sprintf( path, "/tmp/%08x.app", *(u32*)(iTMD+0x1E4+i*0x24) );
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

			for( i=0; i < *(u16*)(iTMD+0x1DE); ++i )
			{
				_sprintf( path, "/tmp/%08x", *(u32*)(iTMD+0x1E4+i*0x24) );
				ISFS_Delete( path );
				_sprintf( path, "/tmp/%08x.app", *(u32*)(iTMD+0x1E4+i*0x24) );
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
				ret = ES_AddContentData( *(s32*)(v[0].data), (u8*)(v[1].data), v[1].len );
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
				for( i=0; i<*(u16*)(iTMD+0x1DE); ++i )
				{
					if( *(u32*)(iTMD+0x1E4+i*0x24) == *(u32*)(v[1].data) )
					{
						if( (*(u16*)(iTMD+0x1EA+i*0x24) & 0x8000) == 0x8000 )
						{
							if( ES_CheckSharedContent( (u8*)(iTMD+0x1F4+i*0x24) ) == 1 )
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
			iTMD = (u8*)malloca( v[0].len, 32 );
			memcpy( iTMD, (u8*)(v[0].data), v[0].len );

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

			dbgprintf("ES:AddTitleStart():%d\n", ret );
		} break;
		case IOCTL_ES_ADDTICKET:
		{			
			_sprintf( path, "/tmp/%08x.tik", *(vu32*)(v[0].data+0x01E0) );

			//Copy ticket to local buffer
			u8 *ticket = (u8*)malloca( v[0].len, 32 );
			memcpy( ticket, (u8*)(v[0].data), v[0].len );

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

			u8 *data = NANDLoadFile( path, size );
			if( data != NULL )
			{
				u32 count=0;
				for( i=0; i < *(u16*)(data+0x1DE); ++i )
				{	
					if( (*(u16*)(data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
					{
						if( ES_CheckSharedContent( (u8*)(data+0x1F4+i*0x24) ) == 1 )
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

			dbgprintf("ES:GetTitleContentsOnCard( %08x-%08x ):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );
		} break;
		case IOCTL_ES_GETTITLECONTENTSCNT:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

			u8 *data = NANDLoadFile( path, size );
			if( data != NULL )
			{
				*(u32*)(v[1].data) = 0;
			
				for( i=0; i < *(u16*)(data+0x1DE); ++i )
				{	
					if( (*(u16*)(data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
					{
						if( ES_CheckSharedContent( (u8*)(data+0x1F4+i*0x24) ) == 1 )
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

			dbgprintf("ES:GetTitleContentCount( %08x-%08x, %d):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_GETTMDVIEWS:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			ret = ES_GetTMDView( iTitleID, (u8*)(v[2].data) );

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
				ret = ES_DIGetTicketView( &TitleID, (u8*)(v[1].data) );
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
					iES_GetTicketView( data + i * 0x2A4, (u8*)(v[2].data) + i * 0xD8 );
				
				free( data );
				ret = ES_SUCCESS;
			}

			dbgprintf("ES:GetTicketViews( %08x-%08x ):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );
		} break;
		case IOCTL_ES_GETTMDVIEWSIZE:
		{
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

			u8 *data = NANDLoadFile( path, size );
			if( data == NULL )
			{
				ret = *size;
			} else {
				*(u32*)(v[1].data) = *(u16*)(data+0x1DE)*16+0x5C;

				free( data );
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
			u8 *data = (u8*)malloca( v[0].len, 0x40 );
			memcpy( data, (u8*)(v[0].data), v[0].len );

			*(u32*)(v[1].data) = *(u16*)(data+0x1DE)*16+0x5C;

			free( data );

			ret = ES_SUCCESS;
			dbgprintf("ES:DIGetTMDViewSize( %d ):%d\n", *(u32*)(v[1].data), ret );
		} break;
		case IOCTL_ES_CLOSECONTENT:
		{
			IOS_Close( *(u32*)(v[0].data) );	//Never returns anything

			ret = ES_SUCCESS;
			//dbgprintf("ES:CloseContent(%d):%d\n", *(u32*)(v[0].data), ret );
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
			memcpy( iTitleID, (u8*)(v[0].data), sizeof(u64) );

			ret = ES_OpenContent( *iTitleID, *(u32*)(v[2].data) );

			//if( ret < 0 )
				dbgprintf("ES:OpenTitleContent( %08x-%08x, %d):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(v[2].data), ret );

		} break;
		case IOCTL_ES_GETTITLEDIR:
		{
			_sprintf( path, "/title/%08x/%08x/data", (u32)((*(u64*)(v[0].data))>>32), (u32)(*(u64*)(v[0].data)) );

			memcpy( (u8*)(v[1].data), path, 32 );

			ret = ES_SUCCESS;
			dbgprintf("ES:GetTitleDataDir(%s):%d\n", v[1].data, ret );
		} break;
		case IOCTL_ES_LAUNCH:
		{
			dbgprintf("ES:LaunchTitle( %08x-%08x )\n", (u32)((*(u64*)(v[0].data))>>32), (u32)(*(u64*)(v[0].data)) );

			ret = ES_LaunchTitle( (u64*)(v[0].data), (u8*)(v[1].data) );

			dbgprintf("ES_LaunchTitle Failed with:%d\n", ret );

		} break;
		case IOCTL_ES_SETUID:
		{
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
						if( ret < 0 )
							dbgprintf("_cc_ahbMemFlush( %d, %04X ):%d\n", 0xF, *(u16*)(TMD_Data+0x198), ret );
						free( TMD_Data );
					}
				}
			}

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

			dbgprintf("ES:GetStoredTMDSize( %08x-%08x, %d ):%d\n", (u32)((*(u64*)(v[0].data))>>32), (u32)((*(u64*)(v[0].data))), *(u32*)(v[1].data), ret );
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
				ret = ES_DIVerify( &TitleID, (u32*)(v[4].data), (TitleMetaData*)(v[3].data), v[3].len, (char*)(v[2].data), (char*)(v[5].data) );

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
int _main( int argc, char *argv[] )
{
	s32 ret=0;
	struct ipcmessage *message=NULL;
	u8 MessageHeap[0x10];
	u32 MessageQueue=0xFFFFFFFF;

	thread_set_priority( 0, 0x79 );
	thread_set_priority( 0, 0x50 );
	thread_set_priority( 0, 0x79 );

#ifdef DEBUG
	dbgprintf("$IOSVersion: ES: %s %s 64M DEBUG$\n", __DATE__, __TIME__ );
#else
	dbgprintf("$IOSVersion: ES: %s %s 64M Release$\n", __DATE__, __TIME__ );
#endif

	KernelVersion = *(vu32*)0x00003140;
	dbgprintf("ES:KernelVersion:%d\n", KernelVersion );

	dbgprintf("ES:Heap Init...");
	MessageQueue = mqueue_create( MessageHeap, 1 );
	dbgprintf("ok\n");

	device_register( "/dev/es", MessageQueue );

	s32 Timer = TimerCreate( 0, 0, MessageQueue, 0xDEADDEAD );
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

	if( ISFS_IsUSB() == FS_ENOENT2 )
	{
		dbgprintf("ES:Found FS-SD\n");
		ret = device_register("/dev/sdio", MessageQueue );
#ifdef DEBUG
		dbgprintf("ES:DeviceRegister(\"/dev/sdio\"):%d QueueID:%d\n", ret, queueid );
#endif
	} else {
		dbgprintf("ES:Found FS-USB\n");
	}

	ES_BootSystem( &TitleID, &KernelVersion );

	SDStatus = malloca( sizeof(u32), 0x40 );
	*SDStatus = 0x00000002;

	HCR = malloca( sizeof(u32)*0x30, 0x40 );
	memset32( HCR, 0, sizeof(u32)*0x30 );
	
//Used in Ioctlvs
	path		= (char*)malloca(		0x40,  32 );
	size		= (u32*) malloca( sizeof(u32), 32 );
	iTitleID	= (u64*) malloca( sizeof(u64), 32 );
	
	dbgprintf("ES:TitleID:%08x-%08x\n", (u32)((TitleID)>>32), (u32)(TitleID) );

	ret = 0;
	u32 MenuType = 0;

	if( TitleID == 0x0000000100000002LL )
	{
		//Disable SD access for system menu, as it breaks channel/game loading
		if( *SDStatus == 1 )
			*SDStatus = 2;
		ret = SMenuFindOffsets( (void*)0x01330000, 0x003D0000 );
	} else {
		ret = SMenuFindOffsets( (void*)0x00000000, 0x01200000 );
		MenuType = 1;
	}

	if( ret != 0 )
	{
		if( SMenuInit( TitleID, TitleVersion ) )
		{
			if( LoadFont( "/font.bin" ) )
				TimerRestart( Timer, 0, 10000 );
		}
	}

	dbgprintf("ES:looping!\n");

	while (1)
	{
		ret = mqueue_recv( MessageQueue, (void *)&message, 0);
		if( ret != 0 )
		{
			dbgprintf("ES:mqueue_recv(%d) FAILED:%d\n", MessageQueue, ret);
			return 0;
		}
	
		if( (u32)message == 0xDEADDEAD )
		{
			TimerStop( Timer );

			SMenuAddFramebuffer();
			if( MenuType == 0 )
			{
				SMenuDraw();
				SMenuReadPad();
			} else if( MenuType == 1 ) {

				SCheatDraw();
				SCheatReadPad();
			}

			TimerRestart( Timer, 0, 2500 );
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
				} else if( strncmp( message->open.device, "/dev/sdio/slot", 14 ) == 0 ) {
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
				if( message->fd == ES_FD || message->fd == SD_FD )
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
		}
	}

	return 0;
}
