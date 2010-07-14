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
#include "ES.h"
#include "DI.h"

u32 *KeyID=NULL;

u16 TitleVersion;

static u8  *DITicket=NULL;
static u8  *CNTMap=NULL;
static u32 *CNTSize=NULL;
static u32 *CNTMapDirty=NULL;

void ES_Fatal( char *name, u32 line, char *file, s32 error, char *msg )
{
	dbgprintf("\n\n************ ES FATAL ERROR ************\n");
	dbgprintf("Function :%s\n", name );
	dbgprintf("line     :%d\n", line );
	dbgprintf("file     :%s\n", file );
	dbgprintf("error    :%d\n", error );
	dbgprintf("%s\n", msg );
	dbgprintf("************ ES FATAL ERROR ************\n");

	while(1);
}
s32 ES_OpenContent( u64 TitleID, u32 ContentID )
{
	char *path = (char*)malloca( 0x40, 32 );
	u32 *size = (u32*)malloca( 4, 32 );
	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)TitleID );

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		s32 ret = *size;
		free( size );
		free( path );
		return ret;
	}

	u32 i=0;
	s32 ret=0;
	for( i=0; i < *(u16*)(data+0x1DE); ++i )
	{
		if( *(u16*)(data+0x1E8+i*0x24) == ContentID )
		{
			//dbgprintf("ContentCnt:%d Index:%d cIndex:%d type:%04x\n", *(vu16*)(data+0x1DE), *(vu32*)(v[0].data), *(vu16*)(data+0x1E8+i*0x24), *(vu16*)(data+0x1EA+i*0x24) );
			if( (*(u16*)(data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
			{
				ret = ES_GetS1ContentID( (u8*)(data+0x1F4+i*0x24) );
				if( ret >= 0 )
				{
					_sprintf( path, "/shared1/%08x.app", ret );
					ret = IOS_Open( path, 1 );
					//dbgprintf("ES:iOpenContent->IOS_Open(\"%s\"):%d\n", path, ret );
				} else {
					dbgprintf("ES:iOpenContent->Fatal Error: tried to open nonexisting content!\n");
					hexdump( (u8*)(data+0x1F4+i*0x24), 0x14 );
					while(1);
				}

			} else {	// not-shared
				_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)(TitleID>>32), (u32)TitleID, *(u32*)(data+0x1E4+i*0x24) );
				ret = IOS_Open( path, 1 );
				//dbgprintf("ES:iOpenContent->IOS_Open(\"%s\"):%d\n", path, ret );
			}

			break;
		}
	}

	free( data );
	free( size );
	free( path );

	return ret;

}
s32 ES_BootSystem( u64 *TitleID, u32 *KernelVersion )
{
	CNTMap=NULL;
	CNTMapDirty=NULL;
	CNTSize=NULL;
	DITicket=NULL;
	KeyID=NULL;

	u32 LaunchDisc = 0;

	char *path = (char*)malloca( 0x40, 32 );
	u32 *size = (u32*)malloca( 4, 32 );

	CNTSize = (u32*)malloca( 4, 32 );
	CNTMapDirty = (u32*)malloca( 4, 32 );
	*CNTMapDirty = 1;

	if( ES_CheckBootOption( "/sys/launch.sys", TitleID ) == 0 )
	{
		*TitleID = 0x100000002LL;
	}

	dbgprintf("ES:Booting %08x-%08x...\n", (u32)(*TitleID>>32), (u32)*TitleID );

	u32 IOSVersion = 0;
	
	if( (u32)(*TitleID>>32) == 1 && (u32)(*TitleID) != 2 )	//IOS loading requested
	{
		IOSVersion = (u32)(*TitleID);
		LaunchDisc = 1;

	} else {
		//Get tmd from title and build kernelversion

		_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*TitleID>>32), (u32)(*TitleID) );

		u8 *data = NANDLoadFile( path, size );
		if( data == NULL )
		{
			dbgprintf("ES:Failed to open:\"%s\":%d\n", path, *size );
			free( path );
			free( size );
			return ES_FATAL;
		}

		IOSVersion = *(u32*)(data+0x188);
		TitleVersion = *(u16*)(data+0x1DC);

		free( data );
	}

	dbgprintf("ES:IOSVersion:%d\n", IOSVersion );
	dbgprintf("ES:TitleVersion:%d\n", TitleVersion );

	//Load TMD of the requested IOS and build KernelVersion
	_sprintf( path, "/title/00000001/%08x/content/title.tmd", IOSVersion );

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		dbgprintf("ES:Failed to open:\"%s\":%d\n", path, *size );
		free( path );
		free( size );
		return ES_FATAL;
	}

	*KernelVersion = *(u16*)(data+0x1DC);
	*KernelVersion|= IOSVersion<<16;
	SetKernelVersion( *KernelVersion );

	u32 version = GetKernelVersion();
	dbgprintf("ES:KernelVersion:%08X, %d\n", version, (version<<8)>>0x18 );

	free( data );

	//Load modules
	if( IOSVersion < 28 )
		IOSVersion = 35;

	s32 r = ES_LoadModules( IOSVersion );
	dbgprintf("ES:ES_LoadModules(%d):%d\n", IOSVersion, r );

	_sprintf( path, "/sys/disc.sys" );
	
	if( LaunchDisc )
	{
		//Check for disc.sys
		u8 *data = NANDLoadFile( path, size );
		if( data != NULL )
		{
			*TitleID = *(vu64*)data;//Set TitleID to disc Title

			r = LoadPPC( data+0x29A );
			dbgprintf("ES:Disc->LoadPPC(%p):%d\n", data+0x29A, r );

			r = ISFS_Delete( path );
			dbgprintf("ES:Disc->ISFS_Delete(%s):%d\n", path, r );

			free( data );
			free( path );
			free( size );

			return ES_SUCCESS;
		}
	}

	ISFS_Delete( path );

	r = ES_LaunchSYS( TitleID );

	free( path );
	free( size );

	return r;

}
s32 ES_Sign( u64 *TitleID, u8 *data, u32 len, u8 *sign, u8 *cert )
{
	u8 *shac = (u8*)malloca( 0x60, 0x40 );
	u8 *hash = (u8*)malloca( 0x14, 0x40 );
	u32 *Key = (u32*)malloca( sizeof( u32 ), 0x40 );
	char *issuer = (char*)malloca( 0x40, 0x40 );
	u8 *NewCert = (u8*)malloca( 0x180, 0x40 );

	memset32( issuer, 0, 0x40 );

	s32 r = sha1( shac, 0, 0, SHA_INIT, NULL );
	r = sha1( shac, data, len, SHA_FINISH, hash );

	r = CreateKey( Key, 0, 4 );

	r = syscall_74( *Key );
	if( r < 0 )
	{
		dbgprintf("syscall_74():%d\n", r );
		goto ES_Sign_CleanUp;
	}

	_sprintf( issuer, "%s%08x%08x", "AP", (u32)(*TitleID>>32), (u32)(*TitleID) );

	r = syscall_76( *Key, issuer, NewCert );
	if( r < 0 )
	{
		dbgprintf("syscall_76( %d, %p, %p ):%d\n", *Key, issuer, NewCert, r );
		goto ES_Sign_CleanUp;
	}

	r = syscall_75( hash, 0x14, *Key, sign );
	if( r < 0 )
		dbgprintf("syscall_75( %p, %d, %d, %p ):%d\n", hash, 0x14, *Key, sign, r );
	else {
		memcpy( cert, NewCert, 0x180 );
	}

ES_Sign_CleanUp:

	DestroyKey( *Key );

	free( NewCert );
	free( issuer );
	free( shac );
	free( hash );
	free( Key );

	return r;
}
s32 ES_GetTitles( u64 *Titles )
{
	char *path = (char*)malloca( 0x40, 32 );
	u32 *size = (u32*)malloca( 4, 32 );

	_sprintf( path, "/sys/uid.sys");

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		free( path );
		free( size );
		return ES_FATAL;
	}
	
	u32 TitleCount=0;
	u32 i;
	for( i=0; i<*size; i+=12 )
	{
		_sprintf( path, "/title/%08x/%08x/content/title.tmd", *(u32*)(data+i), *(u32*)(data+i+4) );
		s32 fd = IOS_Open( path, 1 );
		if( fd >= 0 )
		{
			Titles[TitleCount++] = *(u64*)(data+i);
			IOS_Close( fd );
		}
	}

	free( data );
	free( path );
	free( size );

	return ES_SUCCESS;
}
s32 ES_GetOwnedTitles( u64 *Titles )
{
	char *path = (char*)malloca( 0x40, 32 );
	u32 *size = (u32*)malloca( 4, 32 );

	_sprintf( path, "/sys/uid.sys");

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		free( path );
		free( size );
		return ES_FATAL;
	}
	
	u32 TitleCount=0;
	u32 i;
	for( i=0; i<*size; i+=12 )
	{
		_sprintf( path, "/ticket/%08x/%08x.tik", *(u32*)(data+i), *(u32*)(data+i+4) );
		s32 fd = IOS_Open( path, 1 );
		if( fd >= 0 )
		{
			Titles[TitleCount++] = *(u64*)(data+i);
			IOS_Close( fd );
		}
	}

	free( data );
	free( path );
	free( size );

	return ES_SUCCESS;
}
/*
	each title with a title.tmd counts as a valid title
	all titles which got installed on the system are in the uid.sys therefore we use that to build the pathes
	and check which are present.
*/
s32 ES_GetNumTitles( u32 *TitleCount )
{
	char *path = (char*)malloca( 0x40, 32 );
	u32 *size = (u32*)malloca( 4, 32 );

	_sprintf( path, "/sys/uid.sys");

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		free( path );
		free( size );
		return ES_FATAL;
	}
	
	*TitleCount=0;
	u32 i;
	for( i=0; i<*size; i+=12 )
	{
		_sprintf( path, "/title/%08x/%08x/content/title.tmd", *(u32*)(data+i), *(u32*)(data+i+4) );
		s32 fd = IOS_Open( path, 1 );
		if( fd >= 0 )
		{
			*TitleCount += 1;
			IOS_Close( fd );
		}
	}

	free( data );
	free( path );
	free( size );

	return ES_SUCCESS;
}
/*
	each title with a .tik counts as a valid title
	all titles which got installed on the system are in the uid.sys therefore we use that to build the pathes
	and check which are present.
*/
s32 ES_GetNumOwnedTitles( u32 *TitleCount )
{
	char *path = (char*)malloca( 0x40, 32 );
	u32 *size = (u32*)malloca( 4, 32 );

	_sprintf( path, "/sys/uid.sys");

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		free( path );
		free( size );
		return ES_FATAL;
	}
	
	*TitleCount=0;
	u32 i;
	for( i=0; i<*size; i+=12 )
	{
		_sprintf( path, "/ticket/%08x/%08x.tik", *(u32*)(data+i), *(u32*)(data+i+4) );
		s32 fd = IOS_Open( path, 1 );
		if( fd >= 0 )
		{
			*TitleCount += 1;
			IOS_Close( fd );
		}
	}

	free( data );
	free( path );
	free( size );

	return ES_SUCCESS;
}
void iES_GetTMDView( TitleMetaData *TMD, u8 *oTMDView )
{
	u32 TMDViewSize = (TMD->ContentCount<<4) + 0x5C;
	u8 *TMDView = (u8*)malloca( TMDViewSize, 32 );
	memset32( TMDView, 0, TMDViewSize );

	TMDView[0] = TMD->Version;

	*(u64*)(TMDView+0x04) =	TMD->SystemVersion;
	*(u64*)(TMDView+0x0C) =	TMD->TitleID;
	*(u32*)(TMDView+0x14) = TMD->TitleType;
	*(u16*)(TMDView+0x18) = TMD->GroupID;

	memcpy( TMDView+0x1A, (u8*)TMD + 0x19A, 0x3E );		// Region info

	*(u16*)(TMDView+0x58) = TMD->TitleVersion;
	*(u16*)(TMDView+0x5A) = TMD->ContentCount;

	if( TMD->ContentCount )					// Contents
	{
		int i;
		for( i=0; i < TMD->ContentCount; ++i )
		{
			*(u32*)(TMDView + i * 16 + 0x5C) = TMD->Contents[i].ID;
			*(u16*)(TMDView + i * 16 + 0x60) = TMD->Contents[i].Index;
			*(u16*)(TMDView + i * 16 + 0x62) = TMD->Contents[i].Type;
			*(u64*)(TMDView + i * 16 + 0x64) = TMD->Contents[i].Size;
		}
	}

//region free hack
	memset8( TMDView+0x1E, 0, 16 );
	*(u8*) (TMDView+0x21) = 0x0F;
	*(u16*)(TMDView+0x1C) = 3;
//-

	memcpy( oTMDView, TMDView, TMDViewSize );
	free( TMDView );
}
s32 ES_GetTMDView( u64 *TitleID, u8 *oTMDView )
{
	char *path = (char*)malloca( 0x40, 32 );
	u32 *size = (u32*)malloca( 4, 32 );

	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*TitleID>>32), (u32)*TitleID );

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		free( path );
		free( size );
		return ES_FATAL;
	}

	iES_GetTMDView( (TitleMetaData *)data, oTMDView );

	free( path );
	free( size );

	return ES_SUCCESS;
}
void iES_GetTicketView( u8 *Ticket, u8 *oTicketView )
{
	u8 *TikView = (u8*)malloca( 0xD8, 32 );

	memset32( TikView, 0, 0xD8 );

	TikView[0] = Ticket[0x1BC];

	*(u64*)(TikView+0x04) = *(u64*)(Ticket+0x1D0);
	*(u32*)(TikView+0x0C) = *(u32*)(Ticket+0x1D8);
	*(u64*)(TikView+0x10) = *(u64*)(Ticket+0x1DC);

	memcpy( TikView+0x18, Ticket+0x1E4, 2 );

	*(u16*)(TikView+0x1A) = *(u16*)(Ticket+0x1E6);
	*(u32*)(TikView+0x1C) = *(u32*)(Ticket+0x1E8);
	*(u32*)(TikView+0x20) = *(u32*)(Ticket+0x1EC);
	*(u8*) (TikView+0x24) = *(u8*) (Ticket+0x1F0);

	memcpy( TikView+0x25, Ticket+0x1F1, 0x30 );

	*(u8*) (TikView+0x55) = *(u8*) (Ticket+0x221);

	memcpy( TikView+0x56, Ticket+0x222, 0x40 );

	u32 i;
	for( i=0; i < 7; ++i )
	{
		*(u32*)(i*8 + TikView + 0x98) = *(u32*)(i*8 + Ticket + 0x264);
		*(u32*)(i*8 + TikView + 0x9C) = *(u32*)(i*8 + Ticket + 0x268);
	}

	memcpy( oTicketView, TikView, 0xD8 );

	free( TikView );

	return;
}
s32 ES_DIGetTicketView( u64 *TitleID, u8 *oTikView )
{
	if( DITicket == NULL )
		return ES_FATAL;

	iES_GetTicketView( DITicket, oTikView );

	return ES_SUCCESS;
}
s32 ES_GetUID( u64 *TitleID, u16 *UID )
{
	char *path		= (char*)malloca( 0x40, 32 );
	u32  *size		= (u32*)malloca( sizeof(u32), 32 );

	_sprintf( path, "/sys/uid.sys");

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		if( *size == -106 )		// uid.sys does not exist, create it and add default 1-2 entry
		{
			s32 r = ISFS_CreateFile( path, 0, 3, 3, 3 );
			if( r < 0 )
			{
				dbgprintf("ES:ES_GetUID():Could not create \"/sys/uid.sys\"! Error:%d\n", r );
				free( path );
				free( size );
				return r;
			} else {
				dbgprintf("ES:Created new uid.sys!\n");
				//Create default system menu entry
				s32 fd = IOS_Open( path, 2 );
				if( fd < 0 )
				{
					free( path );
					free( size );
					return fd;
				}

				// 1-2 UID: 0x1000

				*(vu64*)(path)	= 0x0000000100000002LL;
				*(vu32*)(path+8)= 0x00001000;

				IOS_Write( fd, path, 12 );

				IOS_Close( fd );

				_sprintf( path, "/sys/uid.sys");
				data = NANDLoadFile( path, size );
			}

		} else {
			free( path );
			dbgprintf("ES:ES_GetUID():Could not open \"/sys/uid.sys\"! Error:%d\n", *size );
			return *size;
		}
		
	}

	*UID = 0xdead;

	u32 i;
	for( i=0; i < *size/12; ++i )
	{
		if( *(u64*)(data+i*12) == *TitleID )
		{
			*UID = *(u16*)(data+i*12+10);
			break;
		}
	}

	free( data );

	if( *UID == 0xdead )	// Title has no UID yet, add it
	{
		_sprintf( path, "/sys/uid.sys");

		s32 fd = IOS_Open( path, 2 );
		if( fd < 0 )
		{
			free( path );
			free( size );
			return fd;
		}

		// 1-2 UID: 0x1000

		*(vu64*)(path)	= *TitleID;
		*(vu32*)(path+8)= 0x00001000+*size/12+1;

		*UID = 0x1000+*size/12+1;

		dbgprintf("ES:TitleID not found adding new UID:0x%04x\n", *UID );

		s32 r = IOS_Seek( fd, 0, SEEK_END );
		if( r < 0 )
		{
			free( path );
			free( size );
			IOS_Close( fd );
			return r;
		}


		r = IOS_Write( fd, path, 12 );
		if( r != 12 || r < 0 )
		{
			free( path );
			free( size );
			IOS_Close( fd );
			return r;
		}

		IOS_Close( fd );
	}

	free( path );
	free( size );
	return 1;
}
s32 ES_DIVerify( u64 *TitleID, u32 *Key, TitleMetaData *TMD, u32 tmd_size, char *tik, char *Hashes )
{
	char *path		= (char*)malloca( 0x40, 32 );
	u32 *size		= (u32*)malloca( 4, 32 );
	u8 *sTitleID	= (u8*)malloca( 0x10, 32 );
	u8 *encTitleKey	= (u8*)malloca( 0x10, 32 );

	u16 UID = 0;

	_sprintf( path, "/title/%08x/%08x/data", TMD->TitleID >> 32, (u32)(TMD->TitleID) );

	//Create data and content dir if neccessary
	s32 r = ISFS_GetUsage( path, NULL, NULL );
	switch( r )
	{
		case FS_ENOENT2:
		{
			//Create folders!
			_sprintf( path, "/title/%08x",  TMD->TitleID >> 32 );
			if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
			{
				r = ISFS_CreateDir( path, 0, 3, 3, 3 );
				if( r < 0 )
				{
					dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, r );
					goto ES_DIVerfiy_end;
				}
			}

			_sprintf( path, "/title/%08x/%08x", TMD->TitleID >> 32, (u32)(TMD->TitleID) );
			if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
			{
				r = ISFS_CreateDir( path, 0, 3, 3, 3 );
				if( r < 0 )
				{
					dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, r );
					goto ES_DIVerfiy_end;
				}
			}

			_sprintf( path, "/title/%08x/%08x/data", TMD->TitleID >> 32, (u32)(TMD->TitleID) );
			if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
			{
				r = ISFS_CreateDir( path, 0, 3, 3, 3 );
				if( r < 0 )
				{
					dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, r );
					goto ES_DIVerfiy_end;
				}
			}

			_sprintf( path, "/title/%08x/%08x/content", TMD->TitleID >> 32, (u32)(TMD->TitleID) );
			if( ISFS_GetUsage( path, NULL, NULL ) == FS_ENOENT2 )
			{
				r = ISFS_CreateDir( path, 0, 3, 3, 3 );
				if( r < 0 )
				{
					dbgprintf("ISFS_CreateDir(\"%s\"):%d\n", path, r );
					goto ES_DIVerfiy_end;
				}
			}

		} break;
		case FS_SUCCESS:
		{
			dbgprintf("ISFS_GetUsage(\"%s\"):%d\n", path, r );
		} break;
		default:
		{
			dbgprintf("ISFS_GetUsage(\"%s\"):%d\n", path, r );
		} break;
	}


	//Write TMD to nand, disc titles don't write the ticket to nand

	_sprintf( path, "/title/%08x/%08x/content/title.tmd", TMD->TitleID >> 32, (u32)(TMD->TitleID) );

	//Check if there is already a TMD and check its version
	TitleMetaData *nTMD = (TitleMetaData*)NANDLoadFile( path, size );
	if( nTMD != NULL )
	{
		dbgprintf("ES:NAND-TMD:v%d DISC-TMD:v%d\n", nTMD->TitleVersion, TMD->TitleVersion );

		//Check version
		if( nTMD->TitleVersion < TMD->TitleVersion )
		{
			r = NANDWriteFileSafe( path, TMD, tmd_size );
			if( r < 0 )
			{
				dbgprintf("NANDWriteFileSafe(\"%s\"):%d\n", path, r );
				goto ES_DIVerfiy_end;
			}
		}
		free( nTMD );
	} else {
		r = NANDWriteFileSafe( path, TMD, tmd_size );
		if( r < 0 )
		{
			dbgprintf("NANDWriteFileSafe(\"%s\"):%d\n", path, r );
			goto ES_DIVerfiy_end;
		}
	}

	r = CreateKey( Key, 0, 0 );
	if( r < 0 )
	{
		dbgprintf("CreateKey():%d\n", r );
		dbgprintf("keyid:%p:%08x\n", Key, *Key );
		return r;
	}

	r = syscall_71( *Key, 8 );
	if( r < 0 )
	{
		dbgprintf("keyid:%p:%08x\n", Key, *Key );
		dbgprintf("syscall_71():%d\n", r);
		return r;
	}

	memset32( sTitleID, 0, 0x10 );
	memset32( encTitleKey, 0, 0x10 );
	
	*(u64*)sTitleID = TMD->TitleID;
	memcpy( encTitleKey, tik+0x1BF, 0x10 );

	r = syscall_5d( *Key, 0, 4, 1, 0, sTitleID, encTitleKey );
	if( r < 0 )
	{
		dbgprintf("syscall_5d():%d\n", r);
		goto ES_DIVerfiy_end1;
	}

	r = ES_GetUID( &(TMD->TitleID), &UID );
	if( r < 0 )
	{
		dbgprintf("ES_GetUID():%d\n", r );
		goto ES_DIVerfiy_end;
	}

	r = SetUID( 0xF, UID );
	if( r < 0 )
	{
		dbgprintf("SetUID( 0xF, %04X ):%d\n", UID, r );
		goto ES_DIVerfiy_end;
	}

	r = _cc_ahbMemFlush( 0xF, TMD->GroupID );
	if( r < 0 )
	{
		dbgprintf("_cc_ahbMemFlush( %d, %04X ):%d\n", 0xF, TMD->GroupID, r );
		goto ES_DIVerfiy_end;
	}

	DVDLowEnableVideo(0);
	
	r = LoadPPC( (u8*)(TMD) + 0x1BA );
	if( r < 0 )
	{
		dbgprintf("ES:ES_DIVerify->LoadPPC:%d\n", r );
		goto ES_DIVerfiy_end;
	}

	for( r = 0; r < TMD->ContentCount; ++r )
		memcpy( Hashes + r * 20, (u8*)TMD + 0x1F4 + r*0x24, 20 );
	
	DITicket = (u8*)malloca( TICKET_SIZE, 0x40 );
	
	memcpy( DITicket, tik, TICKET_SIZE );

	*TitleID = TMD->TitleID;
	
	//Create /sys/disc.sys
	_sprintf( path, "/sys/space.sys" );
	ISFS_Delete( path );
	_sprintf( path, "/sys/launch.sys" );
	ISFS_Delete( path );
	
	u32 aSize = TMD->ContentCount*0x24 + 0x1E4 + 0xE0;
	
	//dbgprintf("ES:disc.sys size:%d\n", aSize );

	u8 *DiscSys = (u8*)malloca( aSize, 0x40 );
	
	iES_GetTicketView( tik, DiscSys+8 );
	
	memcpy( DiscSys, TitleID, sizeof(u64) );
	memcpy( DiscSys+0xE0, TMD, aSize-0xE0 );
	
	_sprintf( path, "/sys/disc.sys" );
	NANDWriteFileSafe( path, DiscSys, aSize );
	

ES_DIVerfiy_end:
	free( size );
	free( path );

ES_DIVerfiy_end1:
	free( sTitleID );
	free( encTitleKey );

	return r;
}
/*
	Returns the content id for the supplied hash if found

	returns:
		0 < on error
		id on found
*/
s32 ES_GetS1ContentID( void *ContentHash )
{
	if( *CNTMapDirty )
	{
		dbgprintf("ES:Loading content.map...\n");

		if( CNTMap != NULL )
		{
			free( CNTMap );
			CNTMap = NULL;
		}

		CNTMap = NANDLoadFile( "/shared1/content.map", CNTSize );

		if( CNTMap == NULL )
			return ES_FATAL;

		*CNTMapDirty = 0;
	}

	u32 ID=0;
	for( ID=0; ID < *CNTSize/0x1C; ++ID )
	{
		if( memcmp( CNTMap+ID*0x1C+8, ContentHash, 0x14 ) == 0 )
			return ID;
	}
	return ES_FATAL;
}
s32 ES_CheckSharedContent( void *ContentHash )
{
	if( *CNTMapDirty )
	{
		dbgprintf("ES:Loading content.map...\n");

		if( CNTMap != NULL )
		{
			free( CNTMap );
			CNTMap = NULL;
		}

		CNTMap = NANDLoadFile( "/shared1/content.map", CNTSize );

		if( CNTMap == NULL )
			return ES_FATAL;

		*CNTMapDirty = 0;
	}

	u32 ID=0;
	for( ID=0; ID < *CNTSize/0x1C; ++ID )
	{
		if( memcmp( CNTMap+ID*0x1C+8, ContentHash, 0x14 ) == 0 )
			return 1;
	}
	return 0;
}
s32 ES_AddTitleFinish( void *TMD )
{
	char *path		= (char*)malloca( 0x40, 32 );
	char *pathdst	= (char*)malloca( 0x40, 32 );

	s32 r = ES_FATAL;
	u32 i;

	for( i=0; i < *(vu16*)(TMD+0x1DE); ++i )
	{
		if( ((*(vu16*)(TMD+0x1EA+i*0x24)) & 0x8000) == 0x8000 )
		{
			//move to shared1
			switch( ES_CheckSharedContent( (u8*)(TMD+0x1F4+i*0x24) ) )
			{
				case 0:	// Content not yet in shared1 oh-shi
				{
					//get size to calc the name of the new entry
					s32 fd = IOS_Open( "/shared1/content.map", 1|2 );
					s32 size = IOS_Seek( fd, 0, SEEK_END );

					//Now write file to shared1
					_sprintf( path, "/tmp/%08x.app", *(u32*)(TMD+0x1E4+i*0x24) );
					_sprintf( pathdst, "/shared1/%08x.app", size/0x1c );

					r = ISFS_Rename( path, pathdst );

					if( r < 0 )
					{
						dbgprintf("ES:ISFS_Rename( \"%s\", \"%s\" ):%d\n", path, pathdst, r );
						free( path );
						free( pathdst );
						return r;
					}
					
					//add new name
					u8 *Entry = (u8*)malloca( 0x1C, 32 );

					_sprintf( (char*)Entry, "%08x", size/0x1c );
					dbgprintf("Adding new shared1 content:\"%s.app\"\n", Entry );
					memcpy( Entry+8, (u8*)(TMD+0x1F4+i*0x24), 0x14 );

					r = IOS_Write( fd, Entry, 0x1c );

					free( Entry );

					IOS_Close( fd );

					*CNTMapDirty = 1;

					dbgprintf("ES:AddTitleFinish() CID:%08x Installed to shared1 dir\n", *(u32*)(TMD+0x1E4+i*0x24) );

					r = ES_SUCCESS;

				} break;
				case 1:	// content already in shared1, move on
					dbgprintf("ES:AddTitleFinish() CID:%08x already installed!\n", *(u32*)(TMD+0x1E4+i*0x24) );
					_sprintf( path, "/tmp/%08x.app", *(u32*)(TMD+0x1E4+i*0x24) );
					ISFS_Delete( path );
					break;
				default:
					dbgprintf("ES_CheckSharedContent(): failed! \"/shared1/content.map\" missing!\n");
					break;
			}
		} else {
			
			_sprintf( path, "/tmp/%08x.app", *(u32*)(TMD+0x1E4+i*0x24) );
			_sprintf( pathdst, "/title/%08x/%08x/content/%08x.app", *(u32*)(TMD+0x18C), *(u32*)(TMD+0x190), *(u32*)(TMD+0x1E4+i*0x24) );

			s32 fd = IOS_Open(pathdst,1);
			if( fd < 0 )
			{
				r = ISFS_Rename( path, pathdst );
				if( r < 0 )
				{
					dbgprintf("ES:ISFS_Rename( \"%s\", \"%s\" ):%d\n", path, pathdst, r );
					break;
				}

				dbgprintf("ES:AddTitleFinish() CID:%08x Installed to content dir\n", *(u32*)(TMD+0x1E4+i*0x24) );

			} else {
				
				IOS_Close( fd );
				ISFS_Delete( path );

				dbgprintf("ES:AddTitleFinish() CID:%08x already installed!\n", *(u32*)(TMD+0x1E4+i*0x24) );

				r = ES_SUCCESS;
			}
		}
	}

//Copy TMD to content dir
	if( r == ES_SUCCESS )
	{
		_sprintf( path, "/tmp/title.tmd" );
		_sprintf( pathdst, "/title/%08x/%08x/content/title.tmd", *(u32*)(TMD+0x18C), *(u32*)(TMD+0x190) );

		r = ISFS_Rename( path, pathdst );
		if( r < 0 )
			dbgprintf("ISFS_Rename( \"%s\", \"%s\" ):%d\n", path, pathdst, r );
	}

	free( path );
	free( pathdst );
	return r;
}

//s32 ES_CheckSignature( void *data, u32 len )
//{
//	char *path = (char*)malloca( 0x40, 32 );
//
/////this is just for checking signs and stuff
//	u32 *Key = (u64*)malloca( sizeof( u32 )*2, 0x40 );
//	Key[0] = 0;
//
//	s32 r = CreateKey( Key, 1, 2 );
//	dbgprintf("CreateKey( %p, %d, %d ):%d\n", Key, 1, 2, r);
//	dbgprintf("KeyID:%d\n", Key[0] );
//	if( r < 0 )
//	{
//		free( path );
//		return ES_FATAL;
//	}
//
//	u32 *sz = (u64*)malloca( sizeof( u32 ), 0x40 );
//	_sprintf( path, "/sys/cert.sys" );
//
//	u8 *cert_sys = NANDLoadFile( path, sz );
//	if( cert_sys ==  NULL )
//	{
//		DestroyKey( Key[0] );
//		free( path );
//		return ES_FATAL;
//	}
//
//	free( path );
//
//	r = syscall_6f( cert_sys+0x300, 0xFFFFFFF, Key[0] );
//	dbgprintf("syscall_6f( %p, %x, %d ):%d\n", cert_sys+0x300, 0xFFFFFFF, Key[0], r);
//
//	Key[1] = 0;
//	r = CreateKey( Key+1, 1, 2 );
//	dbgprintf("CreateKey( %p, %d, %d ):%d\n", Key+1, 1, 2, r);
//	dbgprintf("KeyID:%d\n", Key[1] );
//
//	r = syscall_6f( cert_sys+0x700, Key[0], Key[1] );
//	dbgprintf("syscall_6f( %p, %x, %d ):%d %08X %08X\n", cert_sys+0x700, Key[0], Key[1], r );
//
//	u8 *SHAc = malloca( 0x60, 0x40 );
//	u8 *hash = malloca( 0x14, 0x40 );
//	u32 *_0 = malloca( sizeof(u32), 0x40 );
//	*_0 = 0;
//
//	r = sha1( SHAc, 0, 0, SHA_INIT, _0 );
//	dbgprintf("sha1( %p, %p, %d, %d, %p ):%d\n", SHAc, 0, 0, SHA_INIT, _0, r );
//
//	r = sha1( SHAc, data+0x140, len-0x140, SHA_FINISH, hash );
//	dbgprintf("sha1( %p, %p, %d, %d, %p ):%d\n", SHAc, data+0x140, len-0x140, SHA_FINISH, hash, r );
//
//	r = syscall_6c( hash, 0x14, Key[1], data+4 );
//	dbgprintf("syscall_6c( %p, %d, %d, %p):%d\n", hash, 0x14, Key[1], data+4, r );
//
//	DestroyKey( Key[0] );
//	DestroyKey( Key[1] );
//
//	free( Key );
//	free( _0 );
//	free( SHAc );
//	free( hash );
//
//
//	return r;
//}
s32 ES_CreateKey( u8 *Ticket )
{
	u8 *sTitleID = (u8*)malloca( 0x10, 0x20 );
	u8 *encTitleKey = (u8*)malloca( 0x10, 0x20 );

	memset32( sTitleID, 0, 0x10 );
	memset32( encTitleKey, 0, 0x10 );

	memcpy( sTitleID, Ticket+0x1DC, 8 );
	memcpy( encTitleKey, Ticket+0x1BF, 0x10 );

	*KeyID=0;

	s32 r = CreateKey( KeyID, 0, 0 );
	if( r < 0 )
	{
		dbgprintf("CreateKey( %p, %d, %d ):%d\n", KeyID, 0, 0, r );
		dbgprintf("KeyID:%d\n", KeyID[0] );
		free( sTitleID );
		free( encTitleKey );
		return r;
	}

	r = syscall_5d( KeyID[0], 0, 4, 1, r, sTitleID, encTitleKey );
	if( r < 0 )
		dbgprintf("syscall_5d( %d, %d, %d, %d, %d, %p, %p ):%d\n", KeyID[0], 0, 4, 1, r, sTitleID, encTitleKey, r );

	free( sTitleID );
	free( encTitleKey );

	return r;
}
s32 doTicketMagic( u8 *Ticket )
{
	u32 *Object = (u32*)malloca( sizeof( u32 )*2, 32 );
	Object[0] = 0;
	Object[1] = 0;

	s32 r = CreateKey( Object, 1, 4 );
	if( r < 0 )
		dbgprintf("CreateKey():%d %p:%08X\n", r, Object, Object[0] );

	r = syscall_5f( Ticket+0x180, 0, Object[0] );
	if( r < 0 )
		dbgprintf("syscall_5f():%d %p:%08X\n", r, Object, Object[0] );

	r = CreateKey( Object+1, 0, 0 );
	if( r < 0 )
		dbgprintf("CreateKey():%d %p:%08X\n", r, Object+1, Object[1] );

	r = syscall_61( 0, Object[0], Object[1] );
	if( r < 0 )
		dbgprintf("syscall_61():%d %08X:%08X\n", r, Object[0], Object[1] );

	u8 *TicketID	= (u8*)malloca( 0x10, 0x20 );
	u8 *decTitleKey = (u8*)malloca( 0x10, 0x20 );
	u8 *encTitleKey = (u8*)malloca( 0x10, 0x20 );

	memset32( TicketID, 0, 0x10 );
	memcpy( TicketID, Ticket+0x1D0, 8 );

	memcpy( encTitleKey, Ticket+0x01BF, 0x10 );

	//hexdump( TicketID, 0x10 );
	//hexdump( encTitleKey, 0x10 );

	//hexdump( Ticket, 0x2a4 );

	r = aes_decrypt_( Object[1], TicketID, encTitleKey, 0x10, decTitleKey );
	if( r < 0 )
		dbgprintf("aes_decrypt_():%d\n", r );
	//hexdump( decTitleKey, 0x10 );

	memcpy( Ticket+0x1BF, decTitleKey, 0x10 );

	DestroyKey( Object[0] );
	DestroyKey( Object[1] );

	free( TicketID );
	free( decTitleKey );
	free( encTitleKey );
	free( Object );

	return ES_SUCCESS;
}
/*
	Decrypt and creata a sha1 for the decrypted data
*/
s32 ES_AddContentFinish( u32 cid, u8 *Ticket, u8 *TMD )
{
	if( Ticket == NULL || TMD == NULL )
		return ES_FATAL;

	char *path = (char*)malloca( 0x40, 32 );
	_sprintf( path, "/tmp/%08x", cid );

	s32 in = IOS_Open( path, 1 );
	if( in < 0 )
	{
		dbgprintf("IOS_Open(\"%s\",1):%d\n", path, in );
		free( path );
		return in;
	}

	_sprintf( path, "/tmp/%08x.app", cid );
	s32 r = ISFS_CreateFile( path, 0, 3, 3, 3 );
	if( r < 0 )
	{
		dbgprintf("ISFS_CreateFile(\"%s\"):%d\n", path, r );
		IOS_Close( in );
		free( path );
		return r;
	}

	s32 out = IOS_Open( path, 2 );
	if( out < 0 )
	{
		dbgprintf("IOS_Open(\"%s\",2):%d\n", path, out );
		IOS_Close( in );
		free( path );
		return out;
	}

	free( path );

//Now decrypt the file
	u8 *block	= (u8*)malloca( 0x4000, 0x40 );
	u8 *iv		= (u8*)malloca( 16, 0x40 );
	memset32( iv, 0, 16 );

//get index which is used as IV
	u32 i;
	u32 FileSize = 0;

	for( i=0; i < *(vu16*)(TMD+0x1DE); ++i )
	{
		if( *(vu32*)(TMD+0x1E4+i*0x24) == cid )
		{
			*(u16*)iv	= *(u16*)(TMD+0x1E8+i*0x24);
			FileSize	= *(u32*)(TMD+0x1F0+i*0x24);
			dbgprintf("ES:Found index:%d Size:%d\n",  *(u16*)(TMD+0x1E8+i*0x24), FileSize );
			break;
		}
	}
	if( FileSize == 0 )
	{
		dbgprintf("FileSize is zero, cid not found in TMD!\n");
		IOS_Close( out );
		IOS_Close( in );
		free( path );
		return -1017;
	}

	u8 *SHA1i = malloca( 0x60, 0x40 );
	u8 *hash = malloca( 0x14, 0x40 );
	u32 *Object = malloca( sizeof(u32), 0x40 );
	*Object = 0;

	r = sha1( SHA1i, 0, 0, SHA_INIT, Object );
	if( r < 0 )
	{
		dbgprintf("sha1():%d\n", r );
		free( SHA1i );
		free( hash );
		free( Object );
		IOS_Close( in );
		IOS_Close( out );
		return r;
	}

	free( Object );

	if((FileSize/0x4000)<1)
	{
		memset32( block, 0, 0x4000 );
		r = IOS_Read( in, block, (FileSize+31)&(~31) );
		if( r < 0 || r < FileSize )
		{
			dbgprintf("IOS_Read( %d, %p, %d):%d\n", in, block, (FileSize+31)&(~31), r );
			r = ES_EHASH;
			goto ACF_Fail;
		}

		r = aes_decrypt_( *KeyID, iv, block, 0x4000, block );
		if(  r < 0 )
			dbgprintf("aes_decrypt( %d, %p, %p %x, %p ):%d\n",  *KeyID, iv, block, 0x4000, block, r );

		r = IOS_Write( out, block, FileSize );
		if( r < 0 || r != FileSize )
			dbgprintf("IOS_Write( %d, %p, %d):%d\n", out, block, FileSize, r );

		r = sha1( SHA1i, block, FileSize, SHA_FINISH, hash );
		if( r < 0 )
			dbgprintf("sha1( %p, %p, %d, %d, %p):%d\n", SHA1i, block, FileSize, SHA_FINISH, hash, r );

	} else {

		for( i=0; i < FileSize/0x4000; ++i)
		{
			memset32( block, 0, 0x4000 );
			r = IOS_Read( in, block, 0x4000 );
			if( r < 0 || r != 0x4000 )
			{
				dbgprintf("IOS_Read( %d, %p, %d):%d\n", in, block, 0x4000, r );
				r = ES_EHASH;
				goto ACF_Fail;
			}
			r = aes_decrypt_( *KeyID, iv, block, 0x4000, block );
			if(  r < 0 )
			{
				dbgprintf("aes_decrypt( %d, %p, %p %x, %p ):%d\n",  *KeyID, iv, block, 0x4000, block, r );
				r = ES_EHASH;
				goto ACF_Fail;
			}
			r = IOS_Write( out, block, 0x4000 );
			if( r < 0 || r != 0x4000 )
			{
				dbgprintf("IOS_Write( %d, %p, %d):%d\n", out, block, 0x4000, r );
				r = ES_EHASH;
				goto ACF_Fail;
			}

			//check if this is the last block
			if( i+1 >= FileSize/0x4000 )
			{
				//is last block; now check if there is some data left

				if( (FileSize%0x4000) == 0 )
				{
					//finish
					r = sha1( SHA1i, block, 0x4000, SHA_FINISH, hash );
					if( r < 0 )
						dbgprintf("sha1( %p, %p, %d, %d, %p):%d\n", SHA1i, block, 0x4000, SHA_FINISH, hash, r );

				} else {
					//just update
					r = sha1( SHA1i, block, 0x4000, SHA_UPDATE, hash );
					if( r < 0 )
						dbgprintf("sha1( %p, %p, %d, %d, %p):%d\n", SHA1i, block, 0x4000, SHA_UPDATE, hash, r );

				}
			} else {
				//just update
				r = sha1( SHA1i, block, 0x4000, SHA_UPDATE, hash );
				if( r < 0 )
					dbgprintf("sha1( %p, %p, %d, %d, %p):%d\n", SHA1i, block, 0x4000, SHA_UPDATE, hash, r );
			}
		}

		if( (FileSize%0x4000) != 0 )
		{
			u32 readsize = (FileSize%0x4000);

			memset32( block, 0, 0x4000 );
			r = IOS_Read( in, block, (readsize+31)&(~31) );
			if( r < 0 || r < readsize )
			{
				dbgprintf("IOS_Read( %d, %p, %d):%d\n", in, block, readsize, r );
				r = ES_EHASH;
				goto ACF_Fail;
			}
			r = aes_decrypt_( *KeyID, iv, block, 0x4000, block );
			if(  r < 0 )
				dbgprintf("aes_decrypt( %d, %p, %p %x, %p ):%d\n",  *KeyID, iv, block, 0x4000, block, r );

			r = IOS_Write( out, block, readsize );
			if( r < 0 || r != readsize )
				dbgprintf("IOS_Write( %d, %p, %d):%d\n", out, block, readsize, r );

			r = sha1( SHA1i, block, readsize, SHA_FINISH, hash );
			if( r < 0 )
				dbgprintf("sha1( %p, %p, %d, %d, %p):%d\n", SHA1i, block, readsize, SHA_FINISH, hash, r );
		}
	}

	r = ES_EHASH;

	//check sha1
	for( i=0; i < *(vu16*)(TMD+0x1DE); ++i )
	{
		if( *(vu32*)(TMD+0x1E4+i*0x24) == cid )
		{
			if( memcmp( hash, (u8*)(TMD+0x1F4+i*0x24), 0x14 ) == 0 )
				r = ES_SUCCESS;
			else {
				dbgprintf("TMD HASH (cid:%d):\n", i);
				hexdump( (u8*)(TMD+0x1F4+i*0x24), 0x14 );
				dbgprintf("CALC HASH:\n");
				hexdump( hash, 0x14 );
			}
			break;
		}
	}

ACF_Fail:

	free( SHA1i );
	free( hash );
	free( iv );
	free( block );

	IOS_Close( in );
	IOS_Close( out );

	return r;
}
s32 ES_AddContentData(s32 cfd, void *data, u32 data_size )
{
	s32 r=0;
	char *path = (char*)malloca( 0x40, 32 );

	_sprintf( path, "/tmp/%08x", cfd );

	s32 fd = IOS_Open( path, 2 );
	if( fd < 0 )
	{
		if( fd == -106 )
		{
			r = ISFS_CreateFile( path, 0, 3, 3, 3 );
			if( r < 0)
			{
				dbgprintf("ISFS_CreateFile(\"%s\"):%d\n", path, r );
				free( path );
				return r;
			}

			fd = IOS_Open( path, 2 );

		} else {
			dbgprintf("IOS_Open(\"%s\"):%d\n", path, fd );
			free( path );
			return fd;
		}

	} else {
		IOS_Seek( fd, 0, SEEK_END );
	}

	r = IOS_Write( fd, data, data_size );
	if( r < 0 || r != data_size )
	{
		dbgprintf("IOS_Write( %d, %p, %d):%d\n", fd, data, data_size, r );
		IOS_Close( fd );
		free( path );
		return r;
	}
	
	IOS_Close( fd );
	free( path );

	return ES_SUCCESS;
}
s32 ES_LoadModules( u32 KernelVersion )
{
	//used later for decrypted
	KeyID = (u32*)malloca( sizeof(u32), 0x40 );

	//load TMD
	char *path = malloca( 0x70, 0x40 );
	
	_sprintf( path, "/title/00000001/%08x/content/title.tmd", KernelVersion );

	u32 *size = (u32*)malloca( sizeof(u32), 0x40 );
	TitleMetaData *TMD = (TitleMetaData*)NANDLoadFile( path, size );
	if( TMD == NULL )
	{
		free( path );
		return *size;
	}

	dbgprintf("ES:ContentCount:%d\n", TMD->ContentCount );

	//Check if di.bin is present

	u32 LoadDI=0;

	_sprintf( path, "/di.bin" );
	u8 *difile = NANDLoadFile( path, size );
	if( difile != NULL )
	{
		free( difile );
		LoadDI=1;
		dbgprintf("ES:Found di.bin\n");
	}

	s32 r=0;
	int i;
	for( i=0; i < TMD->ContentCount; ++i )
	{
		//Don't load boot module
		if( TMD->BootIndex == TMD->Contents[i].Index )
			continue;

		//Skip SD module
		if( TMD->Contents[i].Index == 4 )
		{
			if( ISFS_IsUSB() == FS_ENOENT2 )
				continue;
		}

		//Load special DI module
		if( TMD->Contents[i].Index == 1 && LoadDI )
		{
			_sprintf( path, "/di.bin" );
		} else {
			//check if shared!
			if( TMD->Contents[i].Type & CONTENT_SHARED )
			{
				u32 ID = ES_GetS1ContentID( TMD->Contents[i].SHA1 );

				if( (s32)ID == ES_FATAL )
				{
					dbgprintf("ES:Fatal error: required shared content not found!\n");
					dbgprintf("Hash:\n");
					hexdump( TMD->Contents[i].SHA1, 0x14 );
					while(1);

				} else {
					_sprintf( path, "/shared1/%08x.app", ID );
				}

			} else {
				_sprintf( path, "/title/00000001/%08x/content/%08x.app", KernelVersion, TMD->Contents[i].ID );
			}
		}
		dbgprintf("ES:Loaded Module(%d):\"%s\" ", i, path );
		r = LoadModule( path );
		dbgprintf("ret:%d\n", r );
		if( r < 0 )
		{
			dbgprintf("ES:Fatal error: module failed to start!\n");
			while(1);
		}
	}

	free( size );
	free( TMD );
	free( path );

	thread_set_priority( 0, 0x50 );

	return 0;
}
s32 ES_LaunchTitle( u64 *TitleID, u8 *TikView )
{
	char *path = (char*)malloca( 0x70, 0x40 );

	//if( *TitleID == 0x0000000100000100LL )
	//	*TitleID =  0x0000000100000101LL;

	if( *TitleID == 0x0000000100000100LL )
	{
		_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)((*TitleID)>>32), (u32)(*TitleID) );
		u32 *size = (u32*)malloca( sizeof(32), 32 ); 
		TitleMetaData *TMD = (TitleMetaData *)NANDLoadFile( path, size );
		if( TMD == NULL )
		{
			dbgprintf("ES:Couldn't find TMD of BC!\n");
			_sprintf( path, "/boot2.bin");
			free( size );

		} else {

			_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)((*TitleID)>>32), (u32)(*TitleID), TMD->Contents[ TMD->BootIndex ].ID );
			free( TMD );
			free( size );
		}

		dbgprintf("ES:IOSBoot( %s, 0, %d )\n", path, 0, GetKernelVersion() );

		//now load IOS kernel
		IOSBoot( path, 0, GetKernelVersion() );			
	
		dbgprintf("ES:Booting file failed!\nES:Loading boot2.bin.." );

		_sprintf( path, "/boot2.bin");
		IOSBoot( path, 0, GetKernelVersion() );

		while(1);
	}


	//build launch.sys
	u8 *data=(u8*)malloca( 0xE0, 0x40 );

	memcpy( data, TitleID, sizeof(u64) );
	memcpy( data + sizeof(u64), TikView, 0xD8 );

	_sprintf( path, "/sys/launch.sys" );

	s32 r = NANDWriteFileSafe( path, data, 0xE0 );
	
	free( data );
	free( path );

	dbgprintf("NANDWriteFileSafe():%d\n", r );

	if( r < 0 )
		return r;

	//now load IOS kernel
	if( IOSBoot( "/boot2.bin", 0, GetKernelVersion() ) < 0 )
		return ES_FATAL;

	while(1);
}
s32 ES_CheckBootOption( char *Path, u64 *TitleID )
{
	char *path = (char*)malloca( 0x70, 0x40);
	u32 *size = (u32*)malloca( sizeof(u32), 0x40 );

	_sprintf( path, Path );

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		if( strncmp( path, "/sys/launch.sys", 15 ) == 0 )
			ISFS_Delete( path );

		free( size );
		free( path );
		return 0;
	}

	*TitleID = *(vu64*)data;

	if( strncmp( path, "/sys/launch.sys", 15 ) == 0 )
		ISFS_Delete( path );

	free( size );
	free( path );

	return 1;
}
s32 ES_LaunchSYS( u64 *TitleID )
{
	char *path = malloca( 0x70, 0x40);
	u32 *size = (u32*)malloca( sizeof(u32), 0x40 );
//Load TMD
	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*TitleID>>32), (u32)(*TitleID) );

	u8 *TMD_Data = NANDLoadFile( path, size );
	if( TMD_Data == NULL )
	{
		free( path );
		return *size;
	}

//Load PPC
	s32 r = LoadPPC( TMD_Data+0x1BA );
	dbgprintf("ES:ES_LaunchSYS->LoadPPC:%d\n", r );
	if( r < 0 )
	{
		free( size );
		free( path );
		free( TMD_Data );
		return r;
	}

//Load Ticket
	_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(*TitleID>>32), (u32)(*TitleID) );

	u8 *TIK_Data = NANDLoadFile( path, size );
	if( TIK_Data == NULL )
	{
		free( TMD_Data );
		free( path );
		return *size;
	}

	u16 UID = 0;
	dbgprintf("ES:NANDLoadFile:%p size:%d\n", TIK_Data, *size );

	r = ES_GetUID( TitleID, &UID );
	dbgprintf("ES:ES_GetUID:%d\n", r );
	if( r < 0 )
	{
		free( TIK_Data );
		free( TMD_Data );
		free( size );
		free( path );
		return r;
	}

	r = SetUID( 0xF, UID );
	dbgprintf("ES:SetUID( 0xF, 0x%04X ):%d\n", UID, r );
	if( r < 0 )
	{
		free( TIK_Data );
		free( TMD_Data );
		free( size );
		free( path );
		return r;
	}

	r = _cc_ahbMemFlush( 0xF, *(vu16*)(TMD_Data+0x198) );
	if( r < 0 )
	{
		dbgprintf("_cc_ahbMemFlush( %d, %04X ):%d\n", 0xF, *(vu16*)(TMD_Data+0x198), r );
		free( TIK_Data );
		free( TMD_Data );
		free( size );
		free( path );
		return r;
	}

	if( (s32)((*(vu32*)(TMD_Data+0x1D8)) << 0x1F) > 0 )
		DoStuff(0);
	else
		DoStuff(1);

	//r = DVDLowEnableVideo(1);
	//dbgprintf("DVDLowEnableVideo(1):%d\n", r );
	//if( r < 0 )
	//{
	//	free( TIK_Data );
	//	free( TMD_Data );
	//	free( size );
	//	free( path );
	//	return r;
	//}

//Find boot index
	u32 i;
	for( i=0; i < *(vu16*)(TMD_Data+0x1DE); ++i )
	{
		if( *(vu16*)(TMD_Data+0x1E0) == *(vu16*)(TMD_Data+0x1E8+0x24*i) )
		{
			i = *(vu32*)(TMD_Data+0x1E4+0x24*i);
			break;
		}
	}
	_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)(*TitleID>>32), (u32)(*TitleID), i );

	if( (u32)(*TitleID>>32) == 0x00000001 && (u32)(*TitleID) != 0x00000002 )
	{
		if( *TitleID >= 0x0000000100000100LL )
		{
			if( IOSBoot( path, 0, GetKernelVersion() ) < 0 )
				return ES_FATAL;
		}
	} else {
		dbgprintf("PPCBoot(\"%s\"):", path );
		r = PPCBoot( path );
		dbgprintf("%d\n", r );
	}

	free( TIK_Data );
	free( TMD_Data );
	free( size );
	free( path );

	return 0;
}