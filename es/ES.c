#include "ES.h"

char *path		= (char*)NULL;
u32 *size		= (u32*)NULL;
u64 *iTitleID	= (u64*)NULL;

static u64 TitleID ALIGNED(32);
static u32 KernelVersion ALIGNED(32);
static u32 SkipContent ALIGNED(32);

static u8  *DITicket;

static u8  *CNTMap;
static u32 *CNTSize;
static u32 *CNTMapDirty;

static u64 *TTitles;
static u32 TCount;
static u32 TCountDirty;

u64 *TTitlesO;
u32 TOCount;
u32 TOCountDirty;

u32 *KeyID = (u32*)NULL;

TitleMetaData *iTMD = (TitleMetaData *)NULL;			//used for information during title import
static u8 *iTIK		= (u8 *)NULL;						//used for information during title import

u16 TitleVersion;

DIConfig *DICfg;

// General ES functions

u32 ES_Init( u8 *MessageHeap )
{
//Used in Ioctlvs
	path		= (char*)malloca(		0x40,  32 );
	size		= (u32*) malloca( sizeof(u32), 32 );
	iTitleID	= (u64*) malloca( sizeof(u64), 32 );
	
	CNTMap		= (u8*)NULL;
	DITicket	= (u8*)NULL;
	KeyID		= (u32*)NULL;

	CNTSize		= (u32*)malloca( 4, 32 );
	CNTMapDirty	= (u32*)malloca( 4, 32 );
	*CNTMapDirty= 1;
	
	TTitles			= (u64*)NULL;
	TTitlesO		= (u64*)NULL;
	TCountDirty		= 1;

	TOCount			= 0;
	TOCountDirty	= 0;
	TOCountDirty	= 1;

	u32 MessageQueue = mqueue_create( MessageHeap, 1 );

	device_register( "/dev/es", MessageQueue );

	u32 pid = GetPID();
	SetUID( pid, 0 );
	SetGID( pid, 0 );

	u32 version = KernelGetVersion();
	dbgprintf("ES:KernelVersion:%08X, %d\n", version, (version<<8)>>0x18 );

	ISFS_Init();

	ES_BootSystem();

	dbgprintf("ES:TitleID:%08x-%08x version:%d\n", (u32)((TitleID)>>32), (u32)(TitleID), TitleVersion );

	SMenuInit( TitleID, TitleVersion );
	
	return MessageQueue;
}

s32 ES_BootSystem( void )
{
	char *path	= (char*)malloca( 0x40, 32 );
	u32 *size	= (u32*)malloca( sizeof(u32), 32 );

	u32 LaunchDisc = 0;
	u32 IOSVersion = 0;

	if( CheckBootTitle( "/sys/launch.sys", &TitleID ) == 0 )
	{
		TitleID = 0x100000002LL;
	}

	dbgprintf("ES:Booting %08x-%08x...\n", (u32)(TitleID>>32), (u32)TitleID );

	if( (u32)(TitleID>>32) == 1 && (u32)(TitleID) != 2 )	//	IOS loading requested
	{
		IOSVersion = (u32)(TitleID);
		LaunchDisc = 1;

	} else {

		//get required IOS version from title TMD
		_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)(TitleID) );

		TitleMetaData *TMD = (TitleMetaData *)NANDLoadFile( path, size );
		if( TMD == NULL )
		{
			dbgprintf("ES:Failed to open:\"%s\":%d\n", path, *size );
			free( path );
			free( size );
			PanicBlink( 0, 1,-1);
		}

		IOSVersion		= (u32)TMD->SystemVersion;
		TitleVersion	= TMD->TitleVersion;

		free( TMD );
	}

	dbgprintf("ES:Loading IOS%d ...\n", IOSVersion );

//Load TMD of the requested IOS and build KernelVersion
	_sprintf( path, "/title/00000001/%08x/content/title.tmd", IOSVersion );

	TitleMetaData *TMD = (TitleMetaData *)NANDLoadFile( path, size );
	if( TMD == NULL )
	{
		dbgprintf("ES:Failed to open:\"%s\":%d\n", path, *size );
		free( path );
		free( size );
		PanicBlink( 0, 1,1,-1);
	}

	KernelVersion = TMD->TitleVersion;
	KernelVersion|= IOSVersion<<16;
	KernelSetVersion( KernelVersion );

	u32 version = KernelGetVersion();
	dbgprintf("ES:KernelVersion:%08X, %d\n", version, (version<<8)>>0x18 );

	free( TMD );
	
	//SNEEK does not support single module IOSs so we just load IOS35 instead
	//IOS58 is also not supported
	if( IOSVersion < 28 )
		IOSVersion = 35;
	if( IOSVersion == 58 )
		IOSVersion = 56;

	s32 r = LoadModules( IOSVersion );
	dbgprintf("ES:ES_LoadModules(%d):%d\n", IOSVersion, r );
	if( r < 0 )
	{
		//Network module took too long
		if( r == ES_FATAL )
			LaunchTitle( TitleID );

		PanicBlink( 0, 1,1,1,1,-1 );
	}

	u32 *GameCount = (u32*)malloca( 32, sizeof(u32) );

	if( DVDGetGameCount( GameCount ) == 1 )
	{
        DICfg = (DIConfig *)malloca( *GameCount * DVD_GAMEINFO_SIZE + 0x10, 32 );
        DVDReadGameInfo( 0, *GameCount * DVD_GAMEINFO_SIZE + 0x10, DICfg );
	}

	free( GameCount );

	_sprintf( path, "/sys/disc.sys" );
	
	if( LaunchDisc )
	{
		//Check for disc.sys
		u8 *data = NANDLoadFile( path, size );
		if( data != NULL )
		{
			TitleID = *(vu64*)data;		// Set TitleID to disc Title
			
			if( DICfg->Config & CONFIG_FAKE_CONSOLE_RG )
				Config_ChangeSystem( TitleID, 0 );

			r = LoadPPC( data+0x29A );
			dbgprintf("ES:Disc->LoadPPC(%p):%d\n", data+0x29A, r );

			ISFS_Delete( path );

			free( data );
			free( path );
			free( size );

			return ES_SUCCESS;
		}
	}
	
	ISFS_Delete( path );

	r = LoadTitle( &TitleID );

	free( path );
	free( size );

	return r;

}
s32 LoadModules( u32 IOSVersion )
{
	//used later for decrypted
	KeyID = (u32*)malloca( sizeof(u32), 0x40 );
	char *path = malloca( 0x70, 0x40 );

	u32 LoadDI = false;
	s32 r=0;
	int i;
	
	//load TMD	
	_sprintf( path, "/title/00000001/%08x/content/title.tmd", IOSVersion );

	u32 *size = (u32*)malloca( sizeof(u32), 0x40 );
	TitleMetaData *TMD = (TitleMetaData*)NANDLoadFile( path, size );
	if( TMD == NULL )
	{
		free( path );
		return *size;
	}

	if( TMD->ContentCount == 3 )	// STUB detected!
	{
		dbgprintf("ES:STUB IOS detected, falling back to IOS35\n");
		free( path );
		free( KeyID );
		free( size );
		return LoadModules( 35 );
	}

	dbgprintf("ES:ContentCount:%d\n", TMD->ContentCount );

	//Check if di.bin is present
	_sprintf( path, "/sneek/di.bin" );

	s32 Dfd = IOS_Open( path, 1 );
	if( Dfd >= 0 )
	{
		IOS_Close(Dfd);
		LoadDI = true;
		dbgprintf("ES:Found di.bin\n");
	}

	for( i=0; i < TMD->ContentCount; ++i )
	{
		//Don't load boot module
		if( TMD->BootIndex == TMD->Contents[i].Index )
			continue;

		//Skip SD module if FS module is using SD
		if( TMD->Contents[i].Index == 4 )
		{
			if( ISFS_IsUSB() == FS_ENOENT2 )
				continue;
		}

		//Load special DI module
		if( TMD->Contents[i].Index == 1 && LoadDI )
		{
			_sprintf( path, "/sneek/di.bin" );
		} else {
			//check if shared!
			if( TMD->Contents[i].Type & CONTENT_SHARED )
			{
				u32 ID = GetSharedContentID( TMD->Contents[i].SHA1 );

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
				_sprintf( path, "/title/00000001/%08x/content/%08x.app", IOSVersion, TMD->Contents[i].ID );
			}
		}

		dbgprintf("ES:Loaded Module(%d):\"%s\"\n", i, path );
		r = LoadModule( path );
		if( r < 0 )
		{
			dbgprintf("ES:Fatal error: module failed to start!\n");
			dbgprintf("ret:%d\n", r );
			while(1);
		}
		
		if( TMD->Contents[i].Index == 1 && LoadDI && ISFS_IsUSB() == FS_ENOENT2 )	//Only wait when in SNEEK+DI mode
		{
			dbgprintf("ES:Waiting for DI to init device...");

			while( DVDConnected() != 1 )
				udelay(5000);

			dbgprintf("done!\n");
		}
	}

	dbgprintf("ES:Waiting for network module...\n");


	while( 1 )
	{
		int rfs = IOS_Open("/dev/net/ncd/manage", 0 );
		if( rfs >= 0 )
		{
			IOS_Close(rfs);
			break;
		}

		udelay(500);
	}

	dbgprintf("ES:Network module is up!\n");

	free( size );
	free( TMD );
	free( path );

	thread_set_priority( 0, 0x50 );

	return ES_SUCCESS;
}
s32 LoadTitle( u64 *TitleID )
{
	char *path	= (char *)malloca( 0x70, 32 );
	u32 *size	= (u32*)malloca( sizeof(u32), 32 );

//Load TMD
	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*TitleID>>32), (u32)(*TitleID) );

	TitleMetaData *TMD = (TitleMetaData*)NANDLoadFile( path, size );
	if( TMD == NULL )
	{
		free( path );
		return *size;
	}

//Load PPC
	//s32 r = LoadPPC( TMD + 0x1BA );
	//if( r < 0 )
	//{
	//	dbgprintf("ES:ES_LaunchSYS->LoadPPC:%d\n", r );

	//	free( size );
	//	free( path );
	//	free( TMD );
	//	return r;
	//}

//Load Ticket
	_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(*TitleID>>32), (u32)(*TitleID) );

	u8 *TIK_Data = NANDLoadFile( path, size );
	if( TIK_Data == NULL )
	{
		free( path );
		free( TMD );
		return *size;
	}

	u16 UID = 0;
	dbgprintf("ES:NANDLoadFile:%p size:%d\n", TIK_Data, *size );

	s32 r = GetUID( TitleID, &UID );
	if( r < 0 )
	{
		dbgprintf("ES:ESP_GetUID:%d\n", r );

		free( TIK_Data );
		free( TMD );
		free( size );
		free( path );
		return r;
	}

	r = SetUID( 0xF, UID );
	if( r < 0 )
	{
		dbgprintf("ES:SetUID( 0xF, 0x%04X ):%d\n", UID, r );

		free( TIK_Data );
		free( TMD );
		free( size );
		free( path );
		return r;
	}

	r = SetGID( 0xF, *(vu16*)((u8*)TMD+0x198) );
	if( r < 0 )
	{
		dbgprintf("SetGID( %d, %04X ):%d\n", 0xF, *(vu16*)((u8*)TMD+0x198), r );

		free( TIK_Data );
		free( TMD );
		free( size );
		free( path );
		return r;
	}

//Disable AHB_PROT
	if( TMD->AccessRights & 1 )
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
	if( DICfg->Config & CONFIG_FAKE_CONSOLE_RG )
		Config_ChangeSystem( TMD->TitleID, TMD->TitleVersion );

//Find boot index
	u32 i;
	for( i=0; i < TMD->ContentCount; ++i )
	{
		if( TMD->BootIndex == TMD->Contents[i].Index )
		{
			i = TMD->Contents[i].ID;
			break;
		}
	}
	_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)(*TitleID>>32), (u32)(*TitleID), i );

	if( (u32)(*TitleID>>32) == 0x00000001 && (u32)(*TitleID) != 0x00000002 )
	{
		if( *TitleID >= 0x0000000100000100LL )
		{
			if( IOSBoot( path, 0, KernelGetVersion() ) < 0 )
				return ES_FATAL;
		}
	} else {
		dbgprintf("ES:PPCBoot(\"%s\"):", path );
		r = PPCBoot( path );
		dbgprintf("%d\n", r );
	}

	free( TIK_Data );
	free( TMD );
	free( size );
	free( path );

	return 0;
}
s32 CheckBootTitle( char *Path, u64 *TitleID )
{
	char *path	= (char*)malloca( 0x70, 0x40);
	u32 *size	= (u32*)malloca( sizeof(u32), 0x40 );

	_sprintf( path, Path );

	u8 *data = (u8*)NANDLoadFile( path, size );
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
u64 GetTitleID( void )
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
s32 CreateTitlePath( u64 TitleID )
{
	char *path = (char*)malloca( 0x40, 32 );

	//dbgprintf("Creating path for:%08x-%08x\n", (u32)(TitleID>>32), (u32)(TitleID) );

	_sprintf( path, "/title/%08x/%08x/data", (u32)(TitleID>>32), (u32)(TitleID) );
	if( ISFS_GetUsage( path, (u32*)NULL, (u32*)NULL ) != FS_SUCCESS )
	{
		//dbgprintf("Path \"/title/%08x/%08x/data\" not found\n", (u32)(TitleID>>32), (u32)(TitleID) );

		_sprintf( path, "/title/%08x", (u32)(TitleID>>32) );
		ISFS_CreateDir( path, 0, 3, 3, 3 );
		_sprintf( path, "/title/%08x/%08x", (u32)(TitleID>>32), (u32)(TitleID) );
		ISFS_CreateDir( path, 0, 3, 3, 3 );
		_sprintf( path, "/title/%08x/%08x/data", (u32)(TitleID>>32), (u32)(TitleID) );
		ISFS_CreateDir( path, 0, 3, 3, 3 );
	}
	
	_sprintf( path, "/title/%08x/%08x/content", (u32)(TitleID>>32), (u32)(TitleID) );
	if( ISFS_GetUsage( path, (u32*)NULL, (u32*)NULL ) != FS_SUCCESS )
	{
		ISFS_CreateDir( path, 0, 3, 3, 3 );
	}

	_sprintf( path, "/ticket/%08x", (u32)(TitleID>>32) );
	if( ISFS_GetUsage( path, (u32*)NULL, (u32*)NULL ) != FS_SUCCESS )
	{
		ISFS_CreateDir( path, 0, 3, 3, 3 );
	}

	free( path );

	return ES_SUCCESS;	
}

void iGetTMDView( TitleMetaData *TMD, u8 *oTMDView )
{
	u32 TMDViewSize = (TMD->ContentCount<<4) + 0x5C;
	u8 *TMDView		= (u8*)malloca( TMDViewSize, 32 );

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

////region free hack
//	memset8( TMDView+0x1E, 0, 16 );
//	*(u8*) (TMDView+0x21) = 0x0F;
//	*(u16*)(TMDView+0x1C) = 3;
////-

	memcpy( oTMDView, TMDView, TMDViewSize );
	free( TMDView );
}
s32 GetTMDView( u64 *TitleID, u8 *oTMDView )
{
	char *path	= (char*)malloca( 0x40, 32 );
	u32 *size	= (u32*) malloca( 4, 32 );

	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*TitleID>>32), (u32)*TitleID );

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		free( path );
		free( size );
		return ES_FATAL;
	}

	iGetTMDView( (TitleMetaData *)data, oTMDView );

	free( path );
	free( size );

	return ES_SUCCESS;
}

s32 GetUID( u64 *TitleID, u16 *UID )
{
	char *path	= (char*)malloca( 0x40, 32 );
	u32  *size	= (u32*) malloca( sizeof(u32), 32 );

	_sprintf( path, "/sys/uid.sys");

	UIDSYS *uid = (UIDSYS *)NANDLoadFile( path, size );
	if( uid == NULL )
	{
		if( *size == -106 )		// uid.sys does not exist, create it and add default 1-2 entry
		{
			s32 r = ISFS_CreateFile( path, 0, 3, 3, 3 );
			if( r < 0 )
			{
				dbgprintf("ES:ESP_GetUID():Could not create \"/sys/uid.sys\"! Error:%d\n", r );
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
				uid = (UIDSYS *)NANDLoadFile( path, size );
			}

		} else {
			free( path );
			dbgprintf("ES:ESP_GetUID():Could not open \"/sys/uid.sys\"! Error:%d\n", *size );
			return *size;
		}
		
	}

	*UID = 0xdead;

	u32 i;
	for( i=0; i * 12 < *size; ++i )
	{
		if( uid[i].TitleID == *TitleID )
		{
			*UID = uid[i].GroupID;
			break;
		}
	}

	free( uid );

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
s32 LaunchTitle( u64 TitleID )
{
	u32 majorTitleID = (TitleID) >> 32;
	u32 minorTitleID = (TitleID) & 0xFFFFFFFF;

	char* ticketPath = (char*) malloca(128,32);
	_sprintf(ticketPath,"/ticket/%08x/%08x.tik",majorTitleID,minorTitleID);
	s32 fd = IOS_Open(ticketPath,1);
	free(ticketPath);

	u32 size = IOS_Seek(fd,0,SEEK_END);
	IOS_Seek(fd,0,SEEK_SET);

	u8* ticketData = (u8*) malloca(size,32);
	IOS_Read(fd,ticketData,size);
	IOS_Close(fd);

	u8* ticketView = (u8*) malloca(0xD8,32);
	GetTicketView(ticketData,ticketView);
	free(ticketData);

	s32 r = ESP_LaunchTitle( &TitleID, ticketView );
	free(ticketView);
	return r;
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
/*
	Returns the content id for the supplied hash if found

	returns:
		0 < on error
		id on found
*/
s32 GetSharedContentID( void *ContentHash )
{
	if( *CNTMapDirty )
	{
		dbgprintf("ES:Loading content.map...\n");

		if( CNTMap != NULL )
		{
			free( CNTMap );
			CNTMap = (u8*)NULL;
		}

		CNTMap = (u8*)NANDLoadFile( "/shared1/content.map", CNTSize );

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

void GetTicketView( u8 *Ticket, u8 *oTicketView )
{
	u8 *TikView = (u8*)malloca( 0xD8, 32 );

	memset32( TikView, 0, 0xD8 );

	TikView[0] = Ticket[0x1BC];

	*(u64*)(TikView+0x04) = *(u64*)(Ticket+0x1D0);
	*(u32*)(TikView+0x0C) = *(u32*)(Ticket+0x1D8);
	*(u64*)(TikView+0x10) = *(u64*)(Ticket+0x1DC);
	*(u16*)(TikView+0x18) = *(u16*)(Ticket+0x1E4);
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

s32 doTicketMagic( Ticket *Ticket )
{
	u32 *Object = (u32*)malloca( sizeof( u32 )*2, 32 );
	Object[0] = 0;
	Object[1] = 0;

	s32 r = KeyCreate( Object, 1, 4 );

	syscall_5f( Ticket->DownloadContent, 0, Object[0] );

	KeyCreate( Object+1, 0, 0 );

	syscall_61( 0, Object[0], Object[1] );

	u8 *TicketID	= (u8*)malloca( 0x10, 0x20 );
	u8 *decTitleKey = (u8*)malloca( 0x10, 0x20 );
	u8 *encTitleKey = (u8*)malloca( 0x10, 0x20 );

	memset32( TicketID, 0, 0x10 );
	memcpy( TicketID, &(Ticket->TicketID), 8 );

	memcpy( encTitleKey, Ticket->EncryptedTitleKey, 0x10 );
	r = aes_decrypt_( Object[1], TicketID, encTitleKey, 0x10, decTitleKey );

	memcpy( Ticket->EncryptedTitleKey, decTitleKey, 0x10 );

	KeyDelete( Object[0] );
	KeyDelete( Object[1] );

	free( TicketID );
	free( decTitleKey );
	free( encTitleKey );
	free( Object );

	return ES_SUCCESS;
}

s32 ESP_OpenContent( u64 TitleID, u32 ContentID )
{
	char *path = (char*)malloca( 0x40, 32 );
	u32 *size = (u32*)malloca( 4, 32 );

	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)TitleID );

	TitleMetaData *TMD = (TitleMetaData*)NANDLoadFile( path, size );
	if( TMD == NULL )
	{
		s32 ret = *size;
		free( size );
		free( path );
		return ret;
	}

	u32 i=0;
	s32 ret=0;
	for( i=0; i < TMD->ContentCount; ++i )
	{
		if( TMD->Contents[i].Index == ContentID )
		{
			//dbgprintf("ContentCnt:%d Index:%d cIndex:%d type:%04x\n", *(vu16*)(data+0x1DE), *(vu32*)(v[0].data), *(vu16*)(data+0x1E8+i*0x24), *(vu16*)(data+0x1EA+i*0x24) );
			if( TMD->Contents[i].Type & CONTENT_SHARED )
			{
				ret = GetSharedContentID( TMD->Contents[i].SHA1 );
				if( ret >= 0 )
				{
					_sprintf( path, "/shared1/%08x.app", ret );
					ret = IOS_Open( path, 1 );
					//dbgprintf("ES:iOpenContent->IOS_Open(\"%s\"):%d\n", path, ret );
				} else {
					dbgprintf("ES:iOpenContent->Fatal Error: tried to open nonexisting content!\n");
					hexdump( TMD->Contents[i].SHA1, 0x14 );
					ret = ES_NFOUND;
				}

			} else {	// not-shared
				_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)(TitleID>>32), (u32)TitleID, TMD->Contents[i].ID );
				ret = IOS_Open( path, 1 );
				//dbgprintf("ES:iOpenContent->IOS_Open(\"%s\"):%d\n", path, ret );
			}

			break;
		}
	}

	free( TMD );
	free( size );
	free( path );

	return ret;
}
s32 ESP_LaunchTitle( u64 *TitleID, u8 *TikView )
{
	char *path = (char*)malloca( 0x70, 0x40 );

	//System wants to switch into GC mode
	if( *TitleID == 0x0000000100000100LL )
	{
		_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)((*TitleID)>>32), (u32)(*TitleID) );
		u32 *size = (u32*)malloca( sizeof(32), 32 ); 
		TitleMetaData *TMD = (TitleMetaData *)NANDLoadFile( path, size );
		if( TMD == NULL )
		{
			dbgprintf("ES:Couldn't find TMD of BC!\n");
			_sprintf( path, "/sneek/kernel.bin");
			free( size );

		} else {

			_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)((*TitleID)>>32), (u32)(*TitleID), TMD->Contents[ TMD->BootIndex ].ID );
			free( TMD );
			free( size );
		}

		dbgprintf("ES:IOSBoot( %s, 0, %d )\n", path, 0, KernelGetVersion() );

		//now load IOS kernel
		IOSBoot( path, 0, KernelGetVersion() );			
	
		dbgprintf("ES:Booting file failed!\nES:Loading kernel.bin.." );

		_sprintf( path, "/sneek/kernel.bin");
		IOSBoot( path, 0, KernelGetVersion() );

		PanicBlink( 0, 1,1,1,-1 );
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
	IOSBoot( "/sneek/kernel.bin", 0, KernelGetVersion() );
	
	PanicBlink( 0, 1,1,1,-1 );

	while(1);
}
s32 ESP_DIVerify( u64 *TitleID, u32 *Key, TitleMetaData *TMD, u32 tmd_size, char *tik, char *Hashes )
{
	char *path		= (char*)malloca( 0x40, 32 );
	u32 *size		= (u32*)malloca( 4, 32 );
	u8 *sTitleID	= (u8*)malloca( 0x10, 32 );
	u8 *encTitleKey	= (u8*)malloca( 0x10, 32 );

	u16 UID = 0;

	_sprintf( path, "/title/%08x/%08x/data", (u32)(TMD->TitleID >> 32), (u32)(TMD->TitleID) );

	//Create data and content dir if neccessary
	s32 r = ISFS_GetUsage( path, (u32*)NULL, (u32*)NULL );
	switch( r )
	{
		case FS_ENOENT2:
		{
			CreateTitlePath( TMD->TitleID );
		} break;
		case FS_SUCCESS:
		{
			//;dbgprintf("ES:ISFS_GetUsage(\"%s\"):%d\n", path, r );
		} break;
		default:
		{
			dbgprintf("ES:ISFS_GetUsage(\"%s\"):%d\n", path, r );
		} break;
	}

	//Write TMD to nand, disc titles don't write the ticket to nand
	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TMD->TitleID >> 32), (u32)(TMD->TitleID) );

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
				dbgprintf("ES:NANDWriteFileSafe(\"%s\"):%d\n", path, r );
				goto ES_DIVerfiy_end;
			}
		}
		free( nTMD );
	} else {
		r = NANDWriteFileSafe( path, TMD, tmd_size );
		if( r < 0 )
		{
			dbgprintf("ES:NANDWriteFileSafe(\"%s\"):%d\n", path, r );
			goto ES_DIVerfiy_end;
		}
	}

	r = KeyCreate( Key, 0, 0 );
	if( r < 0 )
	{
		dbgprintf("ES:KeyCreate():%d\n", r );
		dbgprintf("ES:keyid:%p:%08x\n", Key, *Key );
		return r;
	}

	r = KeySetPermissions( *Key, 8 );
	if( r < 0 )
	{
		dbgprintf("ES:keyid:%p:%08x\n", Key, *Key );
		dbgprintf("ES:KeySetPermissions():%d\n", r);
		return r;
	}

	memset32( sTitleID, 0, 0x10 );
	memset32( encTitleKey, 0, 0x10 );
	
	*(u64*)sTitleID = TMD->TitleID;
	memcpy( encTitleKey, tik+0x1BF, 0x10 );

	r = KeyInitialize( *Key, 0, 4, 1, 0, sTitleID, encTitleKey );
	if( r < 0 )
	{
		dbgprintf("ES:KeyInitialize():%d\n", r);
		goto ES_DIVerfiy_end1;
	}

	r = GetUID( &(TMD->TitleID), &UID );
	if( r < 0 )
	{
		dbgprintf("ES:ESP_GetUID():%d\n", r );
		goto ES_DIVerfiy_end;
	}

	r = SetUID( 0xF, UID );
	if( r < 0 )
	{
		dbgprintf("ES:SetUID( 0xF, %04X ):%d\n", UID, r );
		goto ES_DIVerfiy_end;
	}

	r = SetGID( 0xF, TMD->GroupID );
	if( r < 0 )
	{
		dbgprintf("ES:SetGID( %d, %04X ):%d\n", 0xF, TMD->GroupID, r );
		goto ES_DIVerfiy_end;
	}

	DVDLowEnableVideo(0);

	int i;
	for( i = 0; i < TMD->ContentCount; ++i )
		memcpy( Hashes + i * 20, (u8*)TMD + 0x1F4 + i*0x24, 20 );
	
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
	
	GetTicketView( (u8*)tik, DiscSys+8 );
	
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
s32 ESP_DIGetTicketView( u64 *TitleID, u8 *oTikView )
{
	if( DITicket == NULL )
		return ES_FATAL;

	GetTicketView( DITicket, oTikView );

	return ES_SUCCESS;
}
s32 ES_CreateKey( u8 *Ticket )
{
	u8 *sTitleID	= (u8*)malloca( 0x10, 0x20 );
	u8 *encTitleKey = (u8*)malloca( 0x10, 0x20 );

	memset32( sTitleID, 0, 0x10 );
	memset32( encTitleKey, 0, 0x10 );

	memcpy( sTitleID, Ticket+0x1DC, 8 );
	memcpy( encTitleKey, Ticket+0x1BF, 0x10 );

	*KeyID=0;

	s32 r = KeyCreate( KeyID, 0, 0 );
	if( r < 0 )
	{
		dbgprintf("CreateKey( %p, %d, %d ):%d\n", KeyID, 0, 0, r );
		dbgprintf("KeyID:%d\n", KeyID[0] );
		free( sTitleID );
		free( encTitleKey );
		return r;
	}

	r = KeyInitialize( KeyID[0], 0, 4, 1, r, sTitleID, encTitleKey );
	if( r < 0 )
		dbgprintf("syscall_5d( %d, %d, %d, %d, %d, %p, %p ):%d\n", KeyID[0], 0, 4, 1, r, sTitleID, encTitleKey, r );

	free( sTitleID );
	free( encTitleKey );

	return r;
}
/*
	Decrypt and creata a sha1 for the decrypted data
*/
s32 ESP_AddContentFinish( u32 cid, u8 *Ticket, TitleMetaData *TMD )
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
		if( in == ES_NFOUND )
			return ES_SUCCESS;
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

	for( i=0; i < TMD->ContentCount; ++i )
	{
		if( TMD->Contents[i].ID == cid )
		{
			*(u16*)iv	= TMD->Contents[i].Index;
			FileSize	= (u32)TMD->Contents[i].Size;
			dbgprintf("ES:Found CID:%d Size:%d\n",  TMD->Contents[i].Index, FileSize );
			break;
		}
	}

	if( FileSize == 0 )
	{
		dbgprintf("ES:CID not found in TMD!\n");
		IOS_Close( out );
		IOS_Close( in );
		free( path );
		return ES_FATAL;
	}

	u8 *SHA1i = malloca( 0x60, 0x40 );
	u8 *hash = malloca( 0x14, 0x40 );
	u32 *Object = malloca( sizeof(u32), 0x40 );
	*Object = 0;

	r = sha1( SHA1i, 0, 0, SHA_INIT, Object );
	if( r < 0 )
	{
		dbgprintf("ES:sha1():%d\n", r );
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

	// check sha1 for the current content
	for( i=0; i < TMD->ContentCount; ++i )
	{
		if( TMD->Contents[i].ID == cid )
		{
			if( memcmp( hash, TMD->Contents[i].SHA1, 0x14 ) == 0 )
				r = ES_SUCCESS;
			else {
				dbgprintf("ES:Content SHA1 mismatch!\n");
				dbgprintf("ES:TMD HASH (cid:%d):\n", i);
				hexdump( TMD->Contents[i].SHA1, 0x14 );
				dbgprintf("ES:CALC HASH:\n");
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
// ES_Ioctlv functions

s32 ES_Invalid( struct ioctl_vector *Data, u32 In, u32 Out )
{
	dbgprintf("ES:Invalid or unhandled Ioctlv\n" );
	return ES_FATAL;
}

s32 ES_AddTicket( struct ioctl_vector *Data, u32 In, u32 Out )
{
	//Copy ticket to local buffer
	Ticket *ticket = (Ticket*)malloca( Data[0].len, 32 );
	memcpy( ticket, (u8*)(Data[0].data), Data[0].len );
			
	_sprintf( path, "/tmp/%08x.tik", (u32)(ticket->TitleID) );

	if( ticket->ConsoleID )
		doTicketMagic( ticket );

	CreateTitlePath( ticket->TitleID );

	s32 ret = ISFS_CreateFile( path, 0, 3, 3, 3 );
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

			ret = IOS_Write( fd, ticket, Data[0].len );
			if( ret < 0 || ret != Data[0].len )
			{
				dbgprintf("ES:IOS_Write( %d, %p, %d):%d\n", fd, ticket, Data[0].len, ret );

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

	return ret;
}
s32 ES_GetTitleID( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( (u8*)(Data[0].data), &TitleID, sizeof(u64) );

	dbgprintf("ES:GetTitleID(%08x-%08x):0\n", (u32)(TitleID>>32), (u32)(TitleID) );

	return ES_SUCCESS;
}
s32 ES_GetTitleDir( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	_sprintf( path, "/title/%08x/%08x/data", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

	memcpy( (u8*)(Data[1].data), path, 32 );

	dbgprintf("ES:GetTitleDataDir(%s):0\n", Data[1].data );
	return ES_SUCCESS;
}
/*
	each title with a title.tmd counts as a valid title
	all titles which got installed on the system are in the uid.sys therefore we use that to build the pathes
	and check which are present.
*/
s32 ES_GetTitleCount( struct ioctl_vector *Data, u32 In, u32 Out )
{
	if( TCountDirty == 0 )
	{
		*(vu32*)(Data[0].data) = TCount;

	} else {

		_sprintf( path, "/sys/uid.sys");
		UIDSYS *uid = (UIDSYS *)NANDLoadFile( path, size );
		if( uid != NULL )
		{
			TCount = 0;

			u32 i;
			for( i=0; i*12 < *size; i++ )
			{
				_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(uid[i].TitleID>>32), (u32)uid[i].TitleID );
				s32 fd = IOS_Open( path, 1 );
				if( fd >= 0 )
				{
					TCount += 1;
					IOS_Close( fd );
				}
			}

			*(vu32*)(Data[0].data) = TCount;

			free( uid );
		}
	}

	dbgprintf("ES:GetTitleCount(%u):0\n", *(vu32*)(Data[0].data) );
	return ES_SUCCESS;
}
s32 ES_GetTitles( struct ioctl_vector *Data, u32 In, u32 Out )
{
	u64 *Titles = (u64*)(Data[1].data);

	if( TCountDirty == 0 )
	{
		memcpy( Titles, TTitles, sizeof(u64) * TCount );

	} else {

		u32 TitleCount=0;
		u32 i;

		_sprintf( path, "/sys/uid.sys");

		UIDSYS *uid = (UIDSYS *)NANDLoadFile( path, size );
		if( uid != NULL )
		{	
			for( i=0; i*12 < *size; i++ )
			{
				_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(uid[i].TitleID>>32), (u32)uid[i].TitleID );
				s32 fd = IOS_Open( path, 1 );
				if( fd >= 0 )
				{
					Titles[TitleCount++] = uid[i].TitleID;
					IOS_Close( fd );
				}
			}

			free( uid );

			if( TTitles != NULL )
				free(TTitles);

			TTitles = (u64*)malloca( sizeof(u64) * TCount, 32 );

			memcpy( TTitles, Titles, sizeof(u64) * TCount );

			TCountDirty = 0;
		}
	}

	dbgprintf("ES:GetTitles(%u):0\n", TCount );

	return ES_SUCCESS;
}
/*
	each title with a .tik counts as a valid title
	all titles which got installed on the system are in the uid.sys therefore we use that to build the pathes
	and check which are present.
*/
s32 ES_GetOwnedTitleCount( struct ioctl_vector *Data, u32 In, u32 Out )
{
	if( TOCountDirty == 0xdeadbeef )
	{
		*(u32*)(Data[0].data) = TOCount;

	} else {

		TOCount = 0;
		u32 i;

		_sprintf( path, "/sys/uid.sys");

		u8 *data = NANDLoadFile( path, size );
		if( data != NULL )
		{
			for( i=0; i<*size; i+=12 )
			{
				_sprintf( path, "/ticket/%08x/%08x.tik", *(u32*)(data+i), *(u32*)(data+i+4) );
				s32 fd = IOS_Open( path, 1 );
				if( fd >= 0 )
				{
					TOCount += 1;
					IOS_Close( fd );
				}
			}

			free( data );
		}
	
		*(u32*)(Data[0].data) = TOCount;
	}
	
	dbgprintf("ES:GetOwnedTitlesCount(%u):0\n", *(u32*)(Data[0].data) );
	return ES_SUCCESS;
}
s32 ES_GetOwnedTitles( struct ioctl_vector *Data, u32 In, u32 Out )
{
	u64 *Titles = (u64*)(Data[1].data);

	if( TOCountDirty == 0xdeadbeef )
	{
		memcpy( (u64*)(Data[1].data), TTitlesO, sizeof(u64) * TOCount );

	} else {

		u32 TitleCount=0;
		u32 i;

		_sprintf( path, "/sys/uid.sys");

		UIDSYS *uid = (UIDSYS *)NANDLoadFile( path, size );
		if( uid != NULL )
		{

			for( i=0; i*12 < *size; i++ )
			{
				_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(uid[i].TitleID>>32), (u32)uid[i].TitleID );
				s32 fd = IOS_Open( path, 1 );
				if( fd >= 0 )
				{
					Titles[TitleCount++] = uid[i].TitleID;
					IOS_Close( fd );
				}
			}

			free( uid );
	
			if( TTitlesO != NULL )
				free(TTitlesO);

			TTitlesO = (u64*)malloca( sizeof(u64) * TitleCount, 32 );

			memcpy( TTitlesO, Titles, sizeof(u64) * TitleCount );
	
			TOCountDirty = 0xdeadbeef;
		}
	}

	dbgprintf("ES:GetOwnedTitles(%u):0\n", *(u32*)(Data[0].data) );
	return ES_SUCCESS;
}
s32 ES_OpenContent( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret = ESP_OpenContent( TitleID, *(u32*)(Data[0].data) );
	dbgprintf("ES:OpenContent(%d):%d\n", *(u32*)(Data[0].data), ret );

	return ret;
}
s32 ES_OpenTitleContent( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	s32 ret = ESP_OpenContent( *iTitleID, *(u32*)(Data[2].data) );

	dbgprintf("ES:OpenTitleContent( %08x-%08x, %d):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(Data[2].data), ret );

	return ret;
}
s32 ES_ReadContent( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret = IOS_Read( *(u32*)(Data[0].data), (u8*)(Data[1].data), Data[1].len );

	dbgprintf("ES:ReadContent( %d, %p, %d ):%d\n", *(u32*)(Data[0].data), Data[1].data, Data[1].len, ret );

	return ret;
}
s32 ES_SeekContent( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret = IOS_Seek( *(u32*)(Data[0].data), *(u32*)(Data[1].data), *(u32*)(Data[2].data) );
	
	dbgprintf("ES:SeekContent( %d, %d, %d ):%d\n", *(u32*)(Data[0].data), *(u32*)(Data[1].data), *(u32*)(Data[2].data), ret );

	return ret;
}
s32 ES_CloseContent( struct ioctl_vector *Data, u32 In, u32 Out )
{
	IOS_Close( *(u32*)(Data[0].data) );

	dbgprintf("ES:CloseContent(%d):0\n", *(u32*)(Data[0].data) );

	return ES_SUCCESS;
}
s32 ES_AddContentStart( struct ioctl_vector *Data, u32 In, u32 Out )
{
	SkipContent=0;
	s32 ret;

	if( iTMD == NULL )
		ret = ES_FATAL;
	else {
		//check if shared content and if it is already installed so we can skip this one
		u32 i;
		for( i=0; i < iTMD->ContentCount; ++i )
		{
			if( iTMD->Contents[i].ID == *(u32*)(Data[1].data) )
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

		ret = *(u32*)(Data[1].data);
	}

	dbgprintf("ES:AddContentStart():%d\n", ret );
	
	return ret;
}
s32 ES_AddContentData( struct ioctl_vector *Data, u32 In, u32 Out )
{
	if( SkipContent )
	{
		dbgprintf("ES:AddContentData(<fast>):0\n");
		return ES_SUCCESS;
	}

	if( iTMD == NULL )
		return ES_FATAL;

	s32 r=0;
	char *path = (char*)malloca( 0x40, 32 );

	_sprintf( path, "/tmp/%08x", *(u32*)(Data[0].data) );

	s32 fd = IOS_Open( path, 2 );
	if( fd < 0 )
	{
		if( fd == ES_NFOUND )
		{
			r = ISFS_CreateFile( path, 0, 3, 3, 3 );
			if( r < 0 )
			{
				dbgprintf("ES:ISFS_CreateFile(\"%s\"):%d\n", path, r );
				free( path );
				return r;
			}

			fd = IOS_Open( path, 2 );

		} else {
			dbgprintf("ES:IOS_Open(\"%s\"):%d\n", path, fd );
			free( path );
			return fd;
		}

	} else {
		IOS_Seek( fd, 0, SEEK_END );
	}

	r = IOS_Write( fd, (u8*)(Data[1].data), Data[1].len );
	if( r < 0 || r != Data[1].len )
	{
		dbgprintf("ES:IOS_Write( %d, %p, %d):%d\n", fd, (u8*)(Data[1].data), Data[1].len, r );
		IOS_Close( fd );
		free( path );
		return r;
	}
	
	IOS_Close( fd );
	free( path );

	return ES_SUCCESS;
}
s32 ES_AddContentFinish( struct ioctl_vector *Data, u32 In, u32 Out )
{
	if( SkipContent )
	{
		dbgprintf("ES:AddContentFinish(<fast>):0\n");
		return ES_SUCCESS;
	}

	if( iTMD == NULL )
		return ES_FATAL;

	//load Ticket to forge the decryption key
	_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(iTMD->TitleID>>32), (u32)(iTMD->TitleID) );

	s32 ret;
	iTIK = NANDLoadFile( path, size );
	if( iTIK == NULL )
	{
		iCleanUpTikTMD();
		ret = ES_ETIKTMD;

	} else {

		ret = ES_CreateKey( iTIK );
		if( ret >= 0 )
		{
			ret = ESP_AddContentFinish( *(vu32*)(Data[0].data), iTIK, iTMD );
			KeyDelete( *KeyID );
		}

		free( iTIK );
	}	

	dbgprintf("ES:AddContentFinish():%d\n", ret );

	return ret;
}
s32 ES_AddTitleStart( struct ioctl_vector *Data, u32 In, u32 Out )
{
	//Copy TMD to internal buffer for later use
	iTMD = (TitleMetaData*)malloca( Data[0].len, 32 );
	memcpy( iTMD, (u8*)(Data[0].data), Data[0].len );

	_sprintf( path, "/tmp/title.tmd" );

	CreateTitlePath( iTMD->TitleID );

	s32 ret = ISFS_CreateFile( path, 0, 3, 3, 3 );
	if( ret < 0 )
	{
		;//dbgprintf("ISFS_CreateFile(\"%s\"):%d\n", path, ret );
	} else {

		s32 fd = IOS_Open( path, ISFS_OPEN_WRITE );
		if( fd < 0 )
		{
			//dbgprintf("IOS_Open(\"%s\"):%d\n", path, fd );
			ret = fd;
		} else {
			ret = IOS_Write( fd, (u8*)(Data[0].data), Data[0].len );
			if( ret < 0 || ret != Data[0].len )
			{
				;//dbgprintf("IOS_Write( %d, %p, %d):%d\n", fd, Data[0].data, Data[0].len, ret );
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
		GetUID( &(iTMD->TitleID), &UID );
	}

	dbgprintf("ES:AddTitleStart():%d\n", ret );

	return ret;	
}
s32 ES_AddTitleFinish( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 r = ES_FATAL;
	u32 i;	
	char *pathdst	= (char*)malloca( 0x40, 32 );

	CreateTitlePath( iTMD->TitleID );

	for( i=0; i < iTMD->ContentCount; ++i )
	{
		if( iTMD->Contents[i].Type & CONTENT_SHARED )
		{
			//move to shared1
			switch( ES_CheckSharedContent( iTMD->Contents[i].SHA1 ) )
			{
				case 0:	// Content not yet in shared1 oh-shi
				{
					//get size to calc the name of the new entry
					s32 fd = IOS_Open( "/shared1/content.map", 1|2 );
					s32 size = IOS_Seek( fd, 0, SEEK_END );

					//Now write file to shared1
					_sprintf( path, "/tmp/%08x.app", iTMD->Contents[i].ID );
					_sprintf( pathdst, "/shared1/%08x.app", size/0x1c );

					r = ISFS_Rename( path, pathdst );

					if( r < 0 )
					{
						if( !(iTMD->Contents[i].Type & CONTENT_OPTIONAL) )
						{
							dbgprintf("ES:ISFS_Rename( \"%s\", \"%s\" ):%d\n", path, pathdst, r );
							free( path );
							free( pathdst );
							return r;
						} else {
							dbgprintf("ES:Skiping optional content\n");
							continue;
						}
					}
					
					//add new name
					u8 *Entry = (u8*)malloca( 0x1C, 32 );

					_sprintf( (char*)Entry, "%08x", size/0x1c );
					dbgprintf("ES:Adding new shared1 content:\"%s.app\"\n", Entry );
					memcpy( Entry+8, iTMD->Contents[i].SHA1, 0x14 );

					r = IOS_Write( fd, Entry, 0x1C );

					free( Entry );

					IOS_Close( fd );

					*CNTMapDirty = 1;

					dbgprintf("ES:AddTitleFinish() CID:%08x Installed to shared1 dir\n", iTMD->Contents[i].ID );

					r = ES_SUCCESS;

				} break;
				case 1:	// content already in shared1, move on
					dbgprintf("ES:AddTitleFinish() CID:%08x already installed!\n", iTMD->Contents[i].ID );
					_sprintf( path, "/tmp/%08x.app", iTMD->Contents[i].ID );
					ISFS_Delete( path );
					break;
				default:
					dbgprintf("ES_CheckSharedContent(): failed! \"/shared1/content.map\" missing!\n");
					break;
			}
		} else {
			
			_sprintf( path, "/tmp/%08x.app", iTMD->Contents[i].ID );
			_sprintf( pathdst, "/title/%08x/%08x/content/%08x.app", (u32)(iTMD->TitleID>>32), (u32)iTMD->TitleID, iTMD->Contents[i].ID );

			s32 fd = IOS_Open( pathdst, ISFS_OPEN_READ );
			if( fd < 0 )
			{
				r = ISFS_Rename( path, pathdst );
				if( r < 0 )
				{
					if( !(iTMD->Contents[i].Type & CONTENT_OPTIONAL) )
					{
						dbgprintf("ES:ISFS_Rename( \"%s\", \"%s\" ):%d\n", path, pathdst, r );
						break;
					}
					r = ES_SUCCESS;
				}

				dbgprintf("ES:AddTitleFinish() CID:%08x Installed to content dir\n", iTMD->Contents[i].ID );

			} else {
				
				IOS_Close( fd );
				ISFS_Delete( path );

				dbgprintf("ES:AddTitleFinish() CID:%08x already installed!\n", iTMD->Contents[i].ID );

				r = ES_SUCCESS;
			}
		}
	}

//Copy iTMD to content dir
	if( r == ES_SUCCESS )
	{
		_sprintf( path, "/tmp/title.TMD" );
		_sprintf( pathdst, "/title/%08x/%08x/content/title.TMD", (u32)(iTMD->TitleID>>32), (u32)iTMD->TitleID );

		r = ISFS_Rename( path, pathdst );
		if( r < 0 )
			dbgprintf("ISFS_Rename( \"%s\", \"%s\" ):%d\n", path, pathdst, r );
		
		TCountDirty = 1;
		TOCountDirty= 1;
	}

	free( pathdst );

	//Get iTMD for the CID names and delete!
	_sprintf( path, "/tmp/title.TMD" );
	ISFS_Delete( path );

	for( i=0; i < iTMD->ContentCount; ++i )
	{
		_sprintf( path, "/tmp/%08x", iTMD->Contents[i].ID );
		ISFS_Delete( path );
		_sprintf( path, "/tmp/%08x.app", iTMD->Contents[i].ID );
		ISFS_Delete( path );
	}

	iCleanUpTikTMD();

	dbgprintf("ES:AddTitleFinish():%d\n", r );

	return r;
}
s32 ES_AddTitleCancel( struct ioctl_vector *Data, u32 In, u32 Out )
{
	if( iTMD != NULL )
	{
		//Get TMD for the CID names and delete!
		_sprintf( path, "/tmp/title.tmd" );
		ISFS_Delete( path );
			
		u32 i;
		for( i=0; i < iTMD->ContentCount; ++i )
		{
			_sprintf( path, "/tmp/%08x", iTMD->Contents[i].ID );
			ISFS_Delete( path );
			_sprintf( path, "/tmp/%08x.app", iTMD->Contents[i].ID );
			ISFS_Delete( path );
		}

		iCleanUpTikTMD();
	}

	dbgprintf("ES:AddTitleCancel():0\n" );

	return ES_SUCCESS;
}
s32 ES_SetUID( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( &TitleID, (u8*)(Data[0].data), sizeof(u64) );

	u16 UID = 0;
	s32 ret = GetUID( &TitleID, &UID );
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
				ret = SetGID( 0xF, TMD->GroupID );
				if( ret < 0 )
					dbgprintf("SetGID( %d, %04X ):%d\n", 0xF, TMD->GroupID, ret );
				free( TMD );
			}
		} else {
			dbgprintf("ES:SetUID( 0xF, %04X ):%d\n", UID, ret );
		}
	}

	dbgprintf("ES:SetUID(%08x-%08x):%d\n", (u32)(TitleID>>32), (u32)TitleID, ret );

	return ret;
}
/*
	Returns 0 views if title is not installed but always ES_SUCCESS as return
*/
s32 ES_GetTicketViewCount( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

	s32 fd = IOS_Open( path, 1 );
	if( fd < 0 )
	{
		*(u32*)(Data[1].data) = 0;
	} else {
		u32 size = IOS_Seek( fd, 0, SEEK_END );
		*(u32*)(Data[1].data) = size / TICKET_SIZE;
		IOS_Close( fd );
	}

	dbgprintf("ES:GetTicketViewCount( %08x-%08x, %d):0\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(Data[1].data) );
	
	return ES_SUCCESS;
}
s32 ES_GetTicketViews( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret;
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(*iTitleID>>32), (u32)*iTitleID );

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		ret = *size;

	} else  {

		u32 i;
		for( i=0; i < *(u32*)(Data[1].data); ++i )
			GetTicketView( data + i * TICKET_SIZE, (u8*)(Data[2].data) + i * 0xD8 );
				
		free( data );
		ret = ES_SUCCESS;
	}

	dbgprintf("ES:GetTicketViews( %08x-%08x ):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );
	
	return ret;
}
s32 ES_GetTMDViewSize( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret;
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

	TitleMetaData *TMD = (TitleMetaData *)NANDLoadFile( path, size );
	if( TMD == NULL )
	{
		ret = *size;
		*(u32*)(Data[1].data) = 0;
	} else {
		*(u32*)(Data[1].data) = TMD->ContentCount*16+0x5C;

		free( TMD );
		ret = ES_SUCCESS;
	}

	dbgprintf("ES:GetTMDViewSize( %08x-%08x, %d ):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(Data[1].data), ret );

	return ret;
}
s32 ES_GetTMDView( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	s32 ret = GetTMDView( iTitleID, (u8*)(Data[2].data) );

	dbgprintf("ES:GetTMDView( %08x-%08x ):%d\n", (u32)(*iTitleID>>32), (u32)*iTitleID, ret );
	
	return ret;
}
s32 ES_GetTitleContentCount( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret;
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

	TitleMetaData *TMD = (TitleMetaData *)NANDLoadFile( path, size );
	if( TMD != NULL )
	{
		*(u32*)(Data[1].data) = 0;
			
		u32 i;
		for( i=0; i < TMD->ContentCount; ++i )
		{	
			if( TMD->Contents[i].Type & CONTENT_SHARED )
			{
				if( ES_CheckSharedContent( TMD->Contents[i].SHA1 ) == 1 )
					(*(u32*)(Data[1].data))++;
			} else {
				_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)((TMD->TitleID)>>32), (u32)(TMD->TitleID), TMD->Contents[i].ID );
				s32 fd = IOS_Open( path, 1 );
				if( fd >= 0 )
				{
					(*(u32*)(Data[1].data))++;
					IOS_Close( fd );
				}
			}
		}
				
		ret = ES_SUCCESS;
		free( TMD );

	} else {
		ret = *size;
	}

	dbgprintf("ES:GetTitleContentCount( %08x-%08x, %d):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(Data[1].data), ret );
	
	return ret;
}
s32 ES_GetTitleContentsOnCard( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret;
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

	TitleMetaData *tTMD = (TitleMetaData*)NANDLoadFile( path, size );
	if( tTMD != NULL )
	{
		u32 count=0,i;
		for( i=0; i < tTMD->ContentCount; ++i )
		{	
			if( tTMD->Contents[i].Type & CONTENT_SHARED )
			{
				if( ES_CheckSharedContent( tTMD->Contents[i].SHA1 ) == 1 )
				{
					*(u32*)(Data[2].data+0x4*count) = tTMD->Contents[i].ID;
					count++;
				} else {
					dbgprintf("ES:cidx:%u missing shared!:%d\n", ES_CheckSharedContent( tTMD->Contents[i].SHA1 ) );
				}
			} else {
				_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)(tTMD->TitleID>>32), (u32)(tTMD->TitleID), tTMD->Contents[i].ID );
				s32 fd = IOS_Open( path, 1 );
				if( fd >= 0 )
				{
					*(u32*)(Data[2].data+0x4*count) = tTMD->Contents[i].ID;
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
	
	return ret;
}
s32 ES_GetSharedContentCount( struct ioctl_vector *Data, u32 In, u32 Out )
{
	_sprintf( path, "/shared1/content.map" );

	s32 ret;
	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		ret = ES_FATAL;
	} else {

		*(u32*)(Data[0].data) = *size / 0x1C;

		free( data );
		ret = ES_SUCCESS;
	}

	dbgprintf("ES:ES_GetSharedContentCount(%d):%d\n", *(vu32*)(Data[0].data), ret );

	return ret;
}
s32 ES_GetSharedContents( struct ioctl_vector *Data, u32 In, u32 Out )
{
	_sprintf( path, "/shared1/content.map" );
	
	s32 ret;
	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		ret = ES_FATAL;
	} else {

		u32 i;
		for( i=0; i < *(u32*)(Data[0].data); ++i )
			memcpy( (u8*)(Data[1].data+i*20), data+i*0x1C+8, 20 );

		free( data );
		ret = ES_SUCCESS;
	}

	dbgprintf("ES:ES_GetSharedContents():%d\n", ret );

	return ret;
	
}
s32 ES_LaunchTitle( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	dbgprintf("ES:LaunchTitle( %08x-%08x )\n", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

	s32 ret = ESP_LaunchTitle( (u64*)(Data[0].data), (u8*)(Data[1].data) );

	dbgprintf("ES_LaunchTitle Failed with:%d\n", ret );	

	return ret;
}
s32 ES_GetConsumption( struct ioctl_vector *Data, u32 In, u32 Out )
{
	*(u32*)(Data[2].data) = 0;

	dbgprintf("ES:ES_GetConsumption():0\n" );

	return ES_SUCCESS;
}
s32 ES_DIVerify( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret = ESP_DIVerify( &TitleID, (u32*)(Data[4].data), (TitleMetaData*)(Data[3].data), Data[3].len, (char*)(Data[2].data), (char*)(Data[5].data) );

	dbgprintf("ES:ES_DIVerify():%d\n", ret );

	return ret;
}
s32 ES_GetBoot2Version( struct ioctl_vector *Data, u32 In, u32 Out )
{
	*(u32*)(Data[0].data) = 4;

	dbgprintf("ES:GetBoot2Version():0\n" );

	return ES_SUCCESS;	
}
s32 ES_GetDeviceCert( struct ioctl_vector *Data, u32 In, u32 Out )
{
	_sprintf( path, "/sys/device.cert" );

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		GetDeviceCert( (void*)(Data[0].data) );
	} else {
		memcpy( (u8*)(Data[0].data), data, 0x180 );
		free( data );
	}

	dbgprintf("ES:GetDeviceCert():0\n" );

	return ES_SUCCESS;

}
s32 ES_GetDeviceID( struct ioctl_vector *Data, u32 In, u32 Out )
{
	_sprintf( path, "/sys/device.cert" );

	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		GetKey( 1, (u8*)(Data[0].data) );
	} else {
			
		u32 value = 0,i;
				
		//Convert from string to value
		for( i=0; i < 8; ++i )
		{
			if( *(u8*)(data+i+0xC6) > '9' )
				value |= ((*(u8*)(data+i+0xC6))-'W')<<((7-i)<<2);	// 'a'-10 = 'W'
				else 
				value |= ((*(u8*)(data+i+0xC6))-'0')<<((7-i)<<2);
		}

		*(u32*)(Data[0].data) = value;
		free( data );
	}

	dbgprintf("ES:ES_GetDeviceID( 0x%08x ):0\n", *(u32*)(Data[0].data) );

	return ES_SUCCESS;
}
s32 ES_ImportBoot( struct ioctl_vector *Data, u32 In, u32 Out )
{
	dbgprintf("ES:ImportBoot():0\n" );
	return ES_SUCCESS;	
}
s32 ES_KoreanKeyCheck( struct ioctl_vector *Data, u32 In, u32 Out )
{
	dbgprintf("ES:KoreanKeyCheck():-1017\n");
	return ES_FATAL;
}
s32 ES_VerifySign( struct ioctl_vector *Data, u32 In, u32 Out )
{
	dbgprintf("ES:VerifySign():0\n" );		
	return ES_SUCCESS;
}
s32 ES_Decrypt( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret = aes_decrypt_( *(u32*)(Data[0].data), (u8*)(Data[1].data), (u8*)(Data[2].data), Data[2].len, (u8*)(Data[4].data) );
	dbgprintf("ES:Decrypt():%d\n", ret );
	return ret;
}
s32 ES_Encrypt( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret = aes_encrypt( *(u32*)(Data[0].data), (u8*)(Data[1].data), (u8*)(Data[2].data), Data[2].len, (u8*)(Data[4].data) );
	dbgprintf("ES:Encrypt():%d\n", ret );
	return ret;
}
s32 ES_GetDITicketViews( struct ioctl_vector *Data, u32 In, u32 Out )
{
	_sprintf( path, "/ticket/%08x/%08x.tik", (u32)(TitleID>>32), (u32)TitleID );

	s32 ret;
	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		ret = ESP_DIGetTicketView( &TitleID, (u8*)(Data[1].data) );
	} else  {

		GetTicketView( data, (u8*)(Data[1].data) );
				
		free( data );
		ret = ES_SUCCESS;
	}

	dbgprintf("ES:GetDITicketViews( %08x-%08x ):%d\n", (u32)(TitleID>>32), (u32)TitleID, ret );

	return ret;
}
s32 ES_Sign( struct ioctl_vector *Data, u32 In, u32 Out )
{
	u8 *shac	= (u8*)malloca( 0x60, 0x40 );
	u8 *hash	= (u8*)malloca( 0x14, 0x40 );
	u32 *Key	= (u32*)malloca( sizeof( u32 ), 0x40 );
	char *issuer= (char*)malloca( 0x40, 0x40 );
	u8 *NewCert	= (u8*)malloca( 0x180, 0x40 );

	memset32( issuer, 0, 0x40 );

	s32 r = sha1( shac, 0, 0, SHA_INIT, NULL );
	r = sha1( shac, (u8*)(Data[0].data), Data[0].len, SHA_FINISH, hash );

	r = KeyCreate( Key, 0, 4 );

	r = syscall_74( *Key );
	if( r < 0 )
	{
		dbgprintf("syscall_74():%d\n", r );
		goto ES_Sign_CleanUp;
	}

	_sprintf( issuer, "%s%08x%08x", "AP", (u32)(TitleID>>32), (u32)(TitleID) );

	r = syscall_76( *Key, issuer, NewCert );
	if( r < 0 )
	{
		dbgprintf("syscall_76( %d, %p, %p ):%d\n", *Key, issuer, NewCert, r );
		goto ES_Sign_CleanUp;
	}

	r = syscall_75( hash, 0x14, *Key, (u8*)(Data[1].data) );
	if( r < 0 )
		dbgprintf("syscall_75( %p, %d, %d, %p ):%d\n", hash, 0x14, *Key, (u8*)(Data[1].data), r );
	else {
		memcpy( (u8*)(Data[2].data), NewCert, 0x180 );
	}

ES_Sign_CleanUp:

	KeyDelete( *Key );

	free( NewCert );
	free( issuer );
	free( shac );
	free( hash );
	free( Key );

	return r;	
}
s32 ES_GetStoredTMDSize( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

	s32 ret;
	s32 fd = IOS_Open( path, 1 );
	if( fd < 0 )
	{
		ret = fd;
		*(u32*)(Data[1].data) = 0;

	} else {

		fstats *status = (fstats*)malloc( sizeof(fstats) );

		ret = ISFS_GetFileStats( fd, status );
		if( ret < 0 )
		{
			*(u32*)(Data[1].data) = 0;
		} else {
			*(u32*)(Data[1].data) = status->Size;
		}

		IOS_Close( fd );
		free( status );
	}

	dbgprintf("ES:GetStoredTMDSize( %08x-%08x, %d ):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), *(u32*)(Data[1].data), ret );

	return ret;	
}
s32 ES_GetStoredTMD( struct ioctl_vector *Data, u32 In, u32 Out )
{
	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)((*(u64*)(Data[0].data))>>32), (u32)((*(u64*)(Data[0].data))) );

	s32 ret;
	s32 fd = IOS_Open( path, 1 );
	if( fd < 0 )
	{
		ret = fd;
	} else {

		ret = IOS_Read( fd, (u8*)(Data[2].data), Data[2].len );
		if( ret == Data[2].len )
			ret = ES_SUCCESS;

		IOS_Close( fd );
	}

	dbgprintf("ES:GetStoredTMD(%08x-%08x):%d\n", (u32)((*(u64*)(Data[0].data))>>32), (u32)((*(u64*)(Data[0].data))), ret );

	return ret;	
}
s32 ES_GetTmdContentsOnCardCount( struct ioctl_vector *Data, u32 In, u32 Out )
{
	*(u32*)(Data[1].data) = 0;
	TitleMetaData *tTMD = (TitleMetaData*)(Data[0].data);
	
	u32 i;
	for( i=0; i < tTMD->ContentCount; ++i )
	{	
		if( tTMD->Contents[i].Type & CONTENT_SHARED )
		{
			if( ES_CheckSharedContent( tTMD->Contents[i].SHA1 ) == 1 )
				(*(u32*)(Data[1].data))++;
		} else {
			_sprintf( path, "/title/%08x/%08x/content/%08x.app", *(u32*)(Data[0].data+0x18C), *(u32*)(Data[0].data+0x190), tTMD->Contents[i].ID );
			s32 fd = IOS_Open( path, 1 );
			if( fd >= 0 )
			{
				(*(u32*)(Data[1].data))++;
				IOS_Close( fd );
			}
		}
	}

	dbgprintf("ES:GetTmdContentsOnCardCount(%d):0\n", *(u32*)(Data[1].data) );

	return ES_SUCCESS;
}
s32 ES_ListTmdContentsOnCard( struct ioctl_vector *Data, u32 In, u32 Out )
{
	u32 count=0,i;

	for( i=0; i < *(u16*)(Data[0].data+0x1DE); ++i )
	{	
		if( (*(u16*)(Data[0].data+0x1EA+i*0x24) & 0x8000) == 0x8000 )
		{
			if( ES_CheckSharedContent( (u8*)(Data[0].data+0x1F4+i*0x24) ) == 1 )
			{
				*(u32*)(Data[2].data+4*count) = *(u32*)(Data[0].data+0x1E4+i*0x24);
				count++;
			}
		} else {
			_sprintf( path, "/title/%08x/%08x/content/%08x.app", *(u32*)(Data[0].data+0x18C), *(u32*)(Data[0].data+0x190), *(u32*)(Data[0].data+0x1E4+i*0x24) );
			s32 fd = IOS_Open( path, 1 );
			if( fd >= 0 )
			{
				*(u32*)(Data[2].data+4*count) = *(u32*)(Data[0].data+0x1E4+i*0x24);
				count++;
				IOS_Close( fd );
			}
		}
	}

	dbgprintf("ES:ListTmdContentsOnCard():0\n" );

	return ES_SUCCESS;
}
s32 ES_DIGetStoredTMDSize( struct ioctl_vector *Data, u32 In, u32 Out )
{
	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)(TitleID) );

	s32 ret;
	s32 fd = IOS_Open( path, 1 );
	if( fd < 0 )
	{
		ret = fd;
	} else {

		fstats *status = (fstats*)malloc( sizeof(fstats) );

		ret = ISFS_GetFileStats( fd, status );
		if( ret < 0 )
		{
			;//dbgprintf("ES:ISFS_GetFileStats(%d, %p ):%d\n", fd, status, ret );
		} else
			*(u32*)(Data[0].data) = status->Size;

		free( status );
	}

	dbgprintf("ES:DIGetStoredTMDSize( %08x-%08x ):%d\n", (u32)(TitleID>>32), (u32)(TitleID), ret );
			
	return ret;
}
s32 ES_DIGetStoredTMD( struct ioctl_vector *Data, u32 In, u32 Out )
{
	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)(TitleID) );

	s32 ret;
	u8 *data = NANDLoadFile( path, size );
	if( data == NULL )
	{
		ret = *size;
	} else {
		memcpy( (u8*)(Data[1].data), data, *size );
				
		ret = ES_SUCCESS;
		free( data );
	}

	dbgprintf("ES:DIGetStoredTMD( %08x-%08x ):%d\n", (u32)(TitleID>>32), (u32)(TitleID), ret );

	return ret;
}
/*
	in: u64 TitleID
	out: TMD
	ret: status
*/
s32 ES_ExportTitleInit( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( iTitleID, (u64*)(Data[0].data), sizeof(u64) );

	_sprintf( path, "/title/%08x/%08x/content/title.tmd", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

	s32 ret;
	iTMD = (TitleMetaData*)NANDLoadFile( path, size );
	if( iTMD == NULL )
	{
		ret = *size;

	} else {

		memcpy( (u8*)(Data[1].data), iTMD, *size );
				
		ret = ES_SUCCESS;
	}

	dbgprintf("ES:ExportTitleInit(%08x-%08x):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );

	return ret;
}
/*
	in: u64 TitleID
		u32 contentID
	ret: handle
*/
s32 ES_ExportContentBegin( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret = ES_FATAL;

	_sprintf( path, "/title/%08x/%08x/content/%08x.app", (u32)(iTMD->TitleID>>32), (u32)(iTMD->TitleID), iTMD->Contents[ *(u32*)(Data[1].data) ].ID );

	ret = IOS_Open( path, 1 );

	dbgprintf("ES:ExportContentBegin():%d\n", ret );

	return ret;
}
s32 ES_ExportContentData( struct ioctl_vector *Data, u32 In, u32 Out )
{
	s32 ret;

	if( IOS_Read( *(s32*)(Data[0].data), (void*)(Data[1].data), Data[1].len ) == Data[1].len )
		ret = ES_SUCCESS;
	else
		ret = ES_FATAL;

	dbgprintf("ES:ExportContentData(%u,%p,%u):%d\n", *(s32*)(Data[0].data), (void*)(Data[1].data), Data[1].len, ret );

	return ret;
}
s32 ES_ExportContentEnd( struct ioctl_vector *Data, u32 In, u32 Out )
{
	IOS_Close( *(s32*)(Data[0].data) );

	dbgprintf("ES:ExportContentEnd(%u):0\n", *(s32*)(Data[0].data) );

	return ES_SUCCESS;
}
s32 ES_ExportTitleDone( struct ioctl_vector *Data, u32 In, u32 Out )
{
	dbgprintf("ES:ExportTitleDone()\n" );

	if( iTMD != NULL )
		free( iTMD );

	return ES_SUCCESS;
}

s32 ES_DeleteTitleContent( struct ioctl_vector *Data, u32 In, u32 Out )
{
	memcpy( iTitleID, (u8*)(Data[0].data), sizeof(u64) );

	_sprintf( path, "/title/%08x/%08x/content", (u32)(*iTitleID>>32), (u32)(*iTitleID) );

	s32 ret = ISFS_Delete( path );
	if( ret >= 0 )
		ISFS_CreateDir( path, 0, 3, 3, 3 );
	
	dbgprintf("ES:DeleteTitleContent(%08x-%08x):%d\n", (u32)(*iTitleID>>32), (u32)(*iTitleID), ret );

	return ret;
}
s32 ES_DeleteTicket( struct ioctl_vector *Data, u32 In, u32 Out )
{
	return ES_SUCCESS;
}
ESIoctlv ESIoctlvs[0x46] = {
	ES_Invalid,				//	0x00
	ES_AddTicket,			//	0x01
	ES_AddTitleStart,		//	0x02
	ES_AddContentStart,		//	0x03
	ES_AddContentData,		//	0x04
	ES_AddContentFinish,	//	0x05
	ES_AddTitleFinish,		//	0x06
	ES_GetDeviceID,			//	0x07
	ES_LaunchTitle,			//	0x08
	ES_OpenContent,			//	0x09
	ES_ReadContent,			//	0x0a
	ES_CloseContent,		//	0x0b
	ES_GetOwnedTitleCount,	//	0x0c
	ES_GetOwnedTitles,		//	0x0d
	ES_GetTitleCount,		//	0x0e
	ES_GetTitles,			//	0x0f
	ES_GetTitleContentCount,//	0x10
	ES_GetTitleContentsOnCard,//0x11
	ES_GetTicketViewCount,	//	0x12
	ES_GetTicketViews,		//	0x13
	ES_GetTMDViewSize,		//	0x14
	ES_GetTMDView,			//	0x15
	ES_GetConsumption,		//	0x16
	ES_Invalid,				//	0x17	ES_DeleteTitle
	ES_DeleteTicket,		//	0x18
	ES_Invalid,				//	0x19	ES_DIGetTmdViewSize
	ES_Invalid,				//	0x1a	ES_DiGetTmdView
	ES_GetDITicketViews,	//	0x1b
	ES_DIVerify,			//	0x1c
	ES_GetTitleDir,			//	0x1d
	ES_GetDeviceCert,		//	0x1e
	ES_ImportBoot,			//	0x1f
	ES_GetTitleID,			//	0x20
	ES_SetUID,				//	0x21
	ES_DeleteTitleContent,	//	0x22
	ES_SeekContent,			//	0x23
	ES_OpenTitleContent,	//	0x24
	ES_Invalid,				//	0x25	ES_LaunchBC
	ES_Invalid,				//	0x26	ES_ExportTitleInit
	ES_ExportContentBegin,	//	0x27
	ES_ExportContentData,	//	0x28
	ES_ExportContentEnd,	//	0x29
	ES_ExportTitleDone,		//	0x2a
	ES_AddTitleStart,		//	0x2b
	ES_Encrypt,				//	0x2c
	ES_Decrypt,				//	0x2d
	ES_GetBoot2Version,		//	0x2e
	ES_AddTitleCancel,		//	0x2f
	ES_Sign,				//	0x30
	ES_VerifySign,			//	0x31
	ES_GetTmdContentsOnCardCount,//	0x32
	ES_ListTmdContentsOnCard,//	0x33
	ES_GetStoredTMDSize,	//	0x34
	ES_GetStoredTMD,		//	0x35
	ES_GetSharedContentCount,//	0x36
	ES_GetSharedContents,	//	0x37
	ES_Invalid,				//	0x38	ES_DeleteSharedContent?
	ES_DIGetStoredTMDSize,	//	0x39
	ES_DIGetStoredTMD,		//	0x3a
	ES_Invalid,				//	0x3b
	ES_Invalid,				//	0x3c
	ES_Invalid,				//	0x3d
	ES_Invalid,				//	0x3e
	ES_Invalid,				//	0x3f
	ES_Invalid,				//	0x40
	ES_Invalid,				//	0x41
	ES_Invalid,				//	0x42
	ES_Invalid,				//	0x43
	ES_Invalid,				//	0x44
	ES_KoreanKeyCheck		//	0x45
};

void ES_IoctlvN( struct ipcmessage *IPCMessage )
{
	if( IPCMessage->ioctlv.command > 0x45 )
	{
		mqueue_ack( IPCMessage, -1017 );
	} else {
#ifdef DEBUG
		s32 ret = ESIoctlvs[ IPCMessage->ioctlv.command ]( IPCMessage->ioctlv.argv, IPCMessage->ioctlv.argc_in, IPCMessage->ioctlv.argc_io );

		if( ret < 0 )
			dbgprintf("ES:ES_Ioctlv() Ioctlv:%02X In:%u Out:%u Data:%p failed with %d\n", IPCMessage->ioctlv.command, IPCMessage->ioctlv.argc_in, IPCMessage->ioctlv.argc_io, IPCMessage->ioctlv.argv, ret );

		mqueue_ack( IPCMessage, ret );
#else
		mqueue_ack( IPCMessage, ESIoctlvs[ IPCMessage->ioctlv.command ]( IPCMessage->ioctlv.argv, IPCMessage->ioctlv.argc_in, IPCMessage->ioctlv.argc_io ) );
#endif
	}
}
