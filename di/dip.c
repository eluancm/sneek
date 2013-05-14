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
#include "dip.h"

static u32 error ALIGNED(32);
static u32 PartitionSize ALIGNED(32);
static u32 DIStatus ALIGNED(32);
static u32 DICover ALIGNED(32);
static u32 ChangeDisc ALIGNED(32);
DIConfig *DICfg;

static char GamePath[64];
static u32 *KeyID ALIGNED(32);

static u8 *FSTable ALIGNED(32);
u32 ApploaderSize=0;
u32 DolSize=0;
u32 DolOffset=0;
u32 FSTableSize=0;
u32 FSTableOffset=0;
u32 DiscType=0;

u32 FCEntry=0;
FileCache FC[FILECACHE_MAX];

int verbose = 0;
u32 Partition = 0;
u32 Motor = 0;
u32 Disc = 0;
u64 PartitionOffset=0;
u32 GameHook=0;

extern u32 FSMode;

void DICfgFlush( DIConfig *cfg )
{
	s32 fd = DVDOpen( "/sneek/diconfig.bin", DWRITE );
	if( fd >= 0 )
	{
		DVDWrite( fd, cfg, DVD_CONFIG_SIZE );
		DVDClose( fd );
	}
}
void Asciify( char *str )
{
	int i=0;
	for( i=0; i < strlen(str); i++ )
		if( str[i] < 0x20 || str[i] > 0x7F )
			str[i] = '_';
}
u32 GetSystemMenuRegion( void )
{
	char *Path = (char*)malloca( 128, 32 );
	u32 Region = EUR;

	sprintf( Path, "/title/00000001/00000002/content/title.tmd" );
	s32 fd = IOS_Open( Path, DREAD );
	if( fd < 0 )
	{
		free( Path );
		return Region;
	} else {
		u32 size = IOS_Seek( fd, 0, 2 );
		char *TMD = (char*)malloca( size, 32 );

		if( IOS_Read( fd, TMD, size ) == size )
		{
			Region = *(u16*)(TMD+0x1DC) & 0xF;
			dbgprintf( DEBUG_INFO, "DIP:SystemMenuRegion:%d\n", Region );
		}

		IOS_Close( fd );

		free( TMD );
	}
	
	free( Path );

	return Region;
}
u32 DVDGetInstalledGamesCount( void )
{
	u32 GameCount = 0;

	//Check SD card for installed games (DML)
	if( FSMode == SNEEK )
	{
		FSMode = UNEEK;
		GameCount = DVDGetInstalledGamesCount();
		dbgprintf( DEBUG_INFO, "DIP:Installed games on SD:%d\n", GameCount );
		FSMode = SNEEK;
	}

	char *Path = (char*)malloca( 128, 32 );

	sprintf( Path, "/games" );
	if( DVDOpenDir( Path ) == FR_OK )
	{
		while( DVDReadDir() == FR_OK )
		{
			if( strstr( DVDDirGetEntryName(), "boot.bin" ) != NULL )
				continue;

			sprintf( Path, "/games/%.31s/sys/boot.bin", DVDDirGetEntryName() );
			s32 fd = DVDOpen( Path, FA_READ );
			if( fd < 0 )
			{
				dbgprintf( DEBUG_WARNING, "DIP:Failed to open:\"%s\":%d\n", Path, fd );
			} else {
				GameCount++;
				DVDClose(fd);
			}
		}
	}

	free( Path );

	return GameCount;	
}

u32 DVDVerifyGames( void )
{
	u32 UpdateGameCache = 0;
	u32 i;
	
	char *Path = (char*)malloca( 128, 32 );

	for( i=0; i < DICfg->Gamecount; ++i )
	{
		//hexdump( DICfg->GameInfo[i], DVD_GAMEINFO_SIZE );

		sprintf( Path, "/games/%.31s/sys/boot.bin", DICfg->GameInfo[i]+DVD_GAME_NAME_OFF );
		s32 fd = DVDOpen( Path, DREAD );
		if( fd < 0 )
		{
			//Check SD aswell (DML)
			if( FSMode == SNEEK )
			{
				FSMode = UNEEK;

				fd = DVDOpen( Path, DREAD );
				if( fd < 0 )
				{
					dbgprintf( DEBUG_WARNING, "DIP:Could not open:\"%s\"\n", Path );
					UpdateGameCache = 1;
					FSMode = SNEEK;
					break;

				} else {
					DVDClose( fd );
				}

				FSMode = SNEEK;

			} else {

				dbgprintf( DEBUG_WARNING, "DIP:Could not open:\"%s\"\n", Path );
				UpdateGameCache = 1;
				break;
			}
		} else {
			DVDClose( fd );
		}
	}

	free( Path );

	return UpdateGameCache;		
}
s32 DVDUpdateCache( u32 ForceUpdate )
{
	u32 UpdateCache = ForceUpdate;
	u32 GameCount	= 0;
	u32 CurrentGame = 0;
	u32 i;
	u32 DMLite		= 0;
	s32 fres		= 0;
	char *Path = (char*)malloca( 128, 32 );

//First check if file exists and create a new one if needed

	sprintf( Path, "/sneek/diconfig.bin" );
	s32 fd = DVDOpen( Path, FA_READ | FA_WRITE );
	if( fd < 0 )
	{
		//No diconfig.bin found, create a new one
		fd = DVDOpen( Path, FA_CREATE_ALWAYS | FA_READ | FA_WRITE );
		switch(fd)
		{
			case DVD_NO_FILE:
			{
				//In this case there is probably no /sneek folder
				sprintf( Path, "/sneek" );
				DVDCreateDir( Path );
				
				sprintf( Path, "/sneek/diconfig.bin" );
				fd = DVDOpen( Path, FA_CREATE_ALWAYS | FA_READ | FA_WRITE );
				if( fd < 0 )
				{
					dbgprintf( DEBUG_ERROR, "DIP:DVDUpdateCache(%d):Failed to open/create diconfig.bin\n", fd );
					free( Path );
					return DI_FATAL;
				}

			} break;
			case DVD_FATAL:
			{
				dbgprintf( DEBUG_ERROR, "DIP:DVDUpdateCache(%d):Failed to open/create diconfig.bin\n", fd );
				free( Path );
				return DI_FATAL;
			} break;
			case DVD_SUCCESS:
			default:
			{
			} break;
		}
		
		//Create default config
		DICfg->Gamecount= 0;
		DICfg->Config	= CONFIG_AUTO_UPDATE_LIST | HOOK_TYPE_OSLEEP;
		DICfg->SlotID	= 0;
		DICfg->Region	= GetSystemMenuRegion();

		DVDWrite( fd, DICfg, DVD_CONFIG_SIZE );

		UpdateCache = 1;

	} else if( DVDGetSize( fd ) < DVD_CONFIG_SIZE ) {
		
		//Create default config
		DICfg->Gamecount= 0;
		DICfg->Config	= CONFIG_AUTO_UPDATE_LIST | HOOK_TYPE_OSLEEP;
		DICfg->SlotID	= 0;
		DICfg->Region	= GetSystemMenuRegion();
		
		DVDSeek( fd, 0, 0 );
		DVDWrite( fd, DICfg, DVD_CONFIG_SIZE );

		UpdateCache = 1;
	}

	DVDSeek( fd, 0, 0 );

//Read current config and verify the diconfig.bin content
	fres = DVDRead( fd, DICfg, DVD_CONFIG_SIZE );
	if( fres != DVD_CONFIG_SIZE )
	{
		dbgprintf( DEBUG_WARNING, "DIP:Failed to read config:%d expected:%d\n", fres, DVD_CONFIG_SIZE );
		DVDClose( fd );

		sprintf( Path, "/sneek/diconfig.bin" );
		DVDDelete(Path);

		free(Path);

		return DVDUpdateCache(1);
	}

//Sanity Check config
	if( DICfg->Gamecount > 9000 || DICfg->SlotID > 9000 || DICfg->Region > LTN )
	{
		dbgprintf( DEBUG_WARNING, "DIP:Warning diconfig.bin is corrupted!\n");

		//Create default config
		DICfg->Gamecount= 0;
		DICfg->Config	= CONFIG_AUTO_UPDATE_LIST | HOOK_TYPE_OSLEEP;
		DICfg->SlotID	= 0;
		DICfg->Region	= GetSystemMenuRegion();
		
		DVDSeek( fd, 0, 0 );
		DVDWrite( fd, DICfg, DVD_CONFIG_SIZE );

		UpdateCache = 1;
	}

	dbgprintf( DEBUG_INFO, "DIP:GameCount:%d SlotID:%d Region:%d Config:%08X\n", DICfg->Gamecount, DICfg->SlotID, DICfg->Region, DICfg->Config );

//Read rest of config

	//Cache count for size calc
	GameCount = DICfg->Gamecount;
	free( DICfg );

	DICfg = (DIConfig*)malloca( GameCount * DVD_GAMEINFO_SIZE + DVD_CONFIG_SIZE, 32 );

	DVDSeek( fd, 0, 0 );
	fres = DVDRead( fd, DICfg, GameCount * DVD_GAMEINFO_SIZE + DVD_CONFIG_SIZE );
	if( fres != GameCount * DVD_GAMEINFO_SIZE + DVD_CONFIG_SIZE )
	{
		dbgprintf( DEBUG_WARNING, "DIP:Failed to read config:%d expected:%d\n", fres, GameCount * DVD_GAMEINFO_SIZE + DVD_CONFIG_SIZE );
		UpdateCache = 1;
	}

	if( DICfg->Config & CONFIG_AUTO_UPDATE_LIST )
		GameCount = DVDGetInstalledGamesCount();
	else
		GameCount = DICfg->Gamecount;

	dbgprintf( DEBUG_INFO, "DIP:Installed games:%d\n", GameCount );

	if( GameCount != DICfg->Gamecount )
		UpdateCache = 1;

//No need to check if we are going to rebuild anyway
	if( UpdateCache == 0 )
	if( DICfg->Config & CONFIG_AUTO_UPDATE_LIST )
		UpdateCache = DVDVerifyGames();

	if( UpdateCache )
	{
		dbgprintf( DEBUG_INFO, "DIP:Updating game cache...\n");

		DVDClose(fd);
		
		sprintf( Path, "/sneek/diconfig.bin" );
		DVDDelete( Path );

		fd = DVDOpen( Path, FA_CREATE_ALWAYS | FA_READ | FA_WRITE );

		DICfg->Gamecount = GameCount;

		DVDWrite( fd, DICfg, DVD_CONFIG_SIZE );

		char *GameInfo = (char*)malloca( DVD_GAMEINFO_SIZE, 32 );

		//Check on USB and on SD(DML)
		for( i=0; i < 2; i++ )
		{
			sprintf( Path, "/games" );
			if( DVDOpenDir( Path ) == FR_OK )
			{
				while( DVDReadDir() == FR_OK )
				{
					if( DVDDirIsFile() )
						continue;
					
					dbgprintf( DEBUG_INFO, "DIP:Adding game[%d]%s:\"%.31s\"...\n", CurrentGame, DMLite?"(SD)":"", DVDDirGetEntryName()  );
					
					sprintf( Path, "/games/%.31s/sys/boot.bin", DVDDirGetEntryName() );
					s32 gi = DVDOpen( Path, FA_READ );
					if( gi < 0 )
					{
						dbgprintf( DEBUG_WARNING, "DIP:Failed to open:\"%s\":%d\n", Path, gi );
					} else {
						if( DVDRead( gi, GameInfo, DVD_GAMEINFO_SIZE ) != DVD_GAMEINFO_SIZE )
						{
							dbgprintf( DEBUG_WARNING, "DIP:Failed to read from the boot.bin!\n");
							DVDClose( gi );

						} else {

							DVDClose( gi );

							memcpy( GameInfo+DVD_GAME_NAME_OFF, DVDDirGetEntryName(), strlen( DVDDirGetEntryName() ) );

							if( DMLite )
								FSMode = SNEEK;

							if( DVDWrite( fd, GameInfo, DVD_GAMEINFO_SIZE ) != DVD_GAMEINFO_SIZE )
							{
								dbgprintf( DEBUG_ERROR, "DIP:Failed to write to the diconfig.bin!\n");
							}

							if( DMLite )
								FSMode = UNEEK;
						}

						CurrentGame++;
					}
				}
			}

			if( FSMode == UNEEK )
				break;

			FSMode = UNEEK;
			DMLite = 1;
		}

		if( DMLite )
			FSMode = SNEEK;

		free( GameInfo );
	}
	
	DVDClose( fd );

//Read new config
	free( DICfg );
	
	sprintf( Path, "/sneek/diconfig.bin" );
	fd = DVDOpen( Path, DREAD );

	//dbgprintf("DIP:ConfigSize:%d\n", DVDGetSize(fd) );

	DICfg = (DIConfig*)malloca( DVDGetSize(fd), 32 );

	DVDRead( fd, DICfg, DVDGetSize(fd) );
	DVDClose(fd);

	free( Path );
	
	return DI_SUCCESS;
}
u32 DMLite = 0;

s32 DVDSelectGame( int SlotID )
{
	GameHook = 0;

	if( DICfg->Gamecount == 0 )
	{
		DICover |= 1;	// set to no disc
		return DI_FATAL;
	}

	if( SlotID < 0 )
		SlotID = 0;

	if( SlotID >= DICfg->Gamecount )
		SlotID = 0;

	char *str = (char *)malloca( 256, 32 );
	//build path
	sprintf( GamePath, "/games/%.31s/", &DICfg->GameInfo[SlotID][0x60] );
	dbgprintf( DEBUG_INFO, "DIP:Set game path to:\"%s\"\n", GamePath );
	
	FSTable = (u8*)NULL;
	ChangeDisc = 1;
	DICover |= 1;
	
	//Get Apploader size
	sprintf( str, "%ssys/apploader.img", GamePath );
	s32 fd = DVDOpen( str, DREAD );
	if( fd < 0 )
	{
		if( FSMode == SNEEK && fd < 0 )
		{
			FSMode = UNEEK;
			DMLite = 1;
			//Write boot info for DML
			s32 bi = DVDOpen( "/games/boot.bin", FA_CREATE_ALWAYS|FA_WRITE );
			if( bi >= 0 )
			{
				DVDWrite( bi, &DICfg->GameInfo[SlotID][0x60], strlen((char*)(&DICfg->GameInfo[SlotID][0x60])) );
				DVDClose( bi );
			}
			fd = DVDOpen( str, DREAD );
		}

		if( fd < 0 )
		{
			dbgprintf( DEBUG_ERROR, "DIP:Could not open \"%s\"\n", str );
			
			if( DMLite )
				FSMode = SNEEK;

			DVDUpdateCache(1);

			free( str );
			return DVDSelectGame(0);
		}
	} else {
		DMLite = 0;
	}
	

	if( DICfg->GameInfo[SlotID][0x1C] == 0xC2 )	//GC Game write info for DM(L)
	{
		char *gpath = (char*)malloca( 256, 32 );

		sprintf( gpath, "/games/%s/game.iso", &DICfg->GameInfo[SlotID][0x60] );

		s32 fdi = DVDOpen( gpath, DREAD );
		if( fdi < 0 )
		{
			//fst mode
			sprintf( gpath, "/games/%s/", &DICfg->GameInfo[SlotID][0x60] );

		} else {
			DVDClose(fdi);
		}

		DML_CFG *dcfg = (DML_CFG*)0x01200000;

		memset32( dcfg, 0, sizeof( DML_CFG ) );

		dcfg->Version		= 0x00000002;
		dcfg->Magicbytes	= 0xD1050CF6;

		dbgprintf( DEBUG_INFO, "DIP:Config:%08X\n", DICfg->Config );
		
		switch( DICfg->Config & CONFIG_DML_VID_MASK )
		{
			case CONFIG_DML_VID_NONE:
				dcfg->VideoMode = DML_VID_NONE;
			break;
			case CONFIG_DML_VID_FORCE:
				dcfg->VideoMode = DML_VID_FORCE;
			break;
			case CONFIG_DML_VID_AUTO:
			default:
				dcfg->VideoMode = DML_VID_AUTO;
			break;
		}

		switch( DICfg->Config & CONFIG_DML_VID_FORCE_MASK )
		{
			case CONFIG_DML_VID_FORCE_PAL50:
				dcfg->VideoMode |= DML_VID_FORCE_PAL50;
			break;
			case CONFIG_DML_VID_FORCE_PAL60:
				dcfg->VideoMode |= DML_VID_FORCE_PAL60;
			break;
			default:
			case CONFIG_DML_VID_FORCE_NTSC:
				dcfg->VideoMode |= DML_VID_FORCE_NTSC;
			break;
		}

		dcfg->Config = DML_CFG_GAME_PATH;
		
		if( DICfg->Config & CONFIG_DML_CHEATS )
			dcfg->Config |= DML_CFG_CHEATS;

		if( DICfg->Config & CONFIG_DML_DEBUGGER )
			dcfg->Config |= DML_CFG_DEBUGGER;

		if( DICfg->Config & CONFIG_DML_DEBUGWAIT )
			dcfg->Config |= DML_CFG_DEBUGWAIT;

		if( DICfg->Config & CONFIG_DML_NMM )
			dcfg->Config |= DML_CFG_NMM;

		if( DICfg->Config & CONFIG_DML_ACTIVITY_LED )
			dcfg->Config |= DML_CFG_ACTIVITY_LED;

		if( DICfg->Config & CONFIG_DML_PADHOOK )
			dcfg->Config |= DML_CFG_PADHOOK;

		if( DICfg->Config & CONFIG_DML_WIDESCREEN )
			dcfg->Config |= DML_CFG_FORCE_WIDE;

		if( DICfg->Config & CONFIG_DML_PROG_PATCH )
			dcfg->VideoMode |= DML_VID_FORCE_PROG;

		memcpy( dcfg->GamePath, gpath, strlen(gpath) );

		free( gpath );
		
		dbgprintf( DEBUG_INFO, "DIP:DML->Config   :%08X\n", dcfg->Config );
		dbgprintf( DEBUG_INFO, "DIP:DML->VideoMode:%08X\n", dcfg->VideoMode );

		dbgprintf( DEBUG_INFO, "DIP:Wrote config for DM(L)\n");
	}

	ApploaderSize = DVDGetSize( fd ) >> 2;
	DVDClose( fd );

	if( DMLite )
		FSMode = SNEEK;


	dbgprintf( DEBUG_INFO, "DIP:apploader size:%08X\n", ApploaderSize<<2 );

	free( str );

	//update di-config
	fd = DVDOpen( "/sneek/diconfig.bin", DWRITE );
	if( fd >= 0 )
	{
		DICfg->SlotID = SlotID;
		DVDWrite( fd, DICfg, DVD_CONFIG_SIZE );
		DVDClose( fd );
	}

	//Init cache
	u32 count = 0;
	for( count=0; count < FILECACHE_MAX; ++count )
	{
		FC[count].File = 0xdeadbeef;
	}

	return DI_SUCCESS;
}

unsigned char sig_fwrite[32] =
{
    0x94, 0x21, 0xFF, 0xD0,
	0x7C, 0x08, 0x02, 0xA6,
	0x90, 0x01, 0x00, 0x34,
	0xBF, 0x21, 0x00, 0x14, 
    0x7C, 0x9B, 0x23, 0x78,
	0x7C, 0xDC, 0x33, 0x78,
	0x7C, 0x7A, 0x1B, 0x78,
	0x7C, 0xB9, 0x2B, 0x78, 
} ;

unsigned char patch_fwrite[144] =
{
    0x7C, 0x85, 0x21, 0xD7, 0x40, 0x81, 0x00, 0x84, 0x3C, 0xE0, 0xCD, 0x00, 0x3D, 0x40, 0xCD, 0x00, 
    0x3D, 0x60, 0xCD, 0x00, 0x60, 0xE7, 0x68, 0x14, 0x61, 0x4A, 0x68, 0x24, 0x61, 0x6B, 0x68, 0x20, 
    0x38, 0xC0, 0x00, 0x00, 0x7C, 0x06, 0x18, 0xAE, 0x54, 0x00, 0xA0, 0x16, 0x64, 0x08, 0xB0, 0x00, 
    0x38, 0x00, 0x00, 0xD0, 0x90, 0x07, 0x00, 0x00, 0x7C, 0x00, 0x06, 0xAC, 0x91, 0x0A, 0x00, 0x00, 
    0x7C, 0x00, 0x06, 0xAC, 0x38, 0x00, 0x00, 0x19, 0x90, 0x0B, 0x00, 0x00, 0x7C, 0x00, 0x06, 0xAC, 
    0x80, 0x0B, 0x00, 0x00, 0x7C, 0x00, 0x04, 0xAC, 0x70, 0x09, 0x00, 0x01, 0x40, 0x82, 0xFF, 0xF4, 
    0x80, 0x0A, 0x00, 0x00, 0x7C, 0x00, 0x04, 0xAC, 0x39, 0x20, 0x00, 0x00, 0x91, 0x27, 0x00, 0x00, 
    0x7C, 0x00, 0x06, 0xAC, 0x74, 0x09, 0x04, 0x00, 0x41, 0x82, 0xFF, 0xB8, 0x38, 0xC6, 0x00, 0x01, 
    0x7F, 0x86, 0x20, 0x00, 0x40, 0x9E, 0xFF, 0xA0, 0x7C, 0xA3, 0x2B, 0x78, 0x4E, 0x80, 0x00, 0x20, 
};
unsigned char sig_iplmovie[] =
{
    0x4D, 0x50, 0x4C, 0x53,
	0x2E, 0x4D, 0x4F, 0x56,
	0x49, 0x45, 0x00, 
} ;

unsigned char patch_iplmovie[] =
{
    0x49, 0x50, 0x4C, 0x2E,
	0x45, 0x55, 0x4C, 0x41,
	0x00, 
};
u32 FSTRebuild( void )
{
	dbgprintf( DEBUG_INFO, "DIP:Rebuilding FST please wait...\n");

	u32 Entries = *(u32*)(FSTable+0x08);
	char *NameOff = (char*)(FSTable + Entries * 0x0C);
	FEntry *fe = (FEntry*)(FSTable);
	
	dbgprintf( DEBUG_INFO,"DIP:FST entries:%u\n", Entries );

	if( Entries > 1000000 )	// something is wrong!
		return 0;

	u32 Entry[16];
	u32 LEntry[16];
	char Path[256];
	u32 level=0;
	u64 CurrentOffset = -1;
	u32 i,j;

	// Find lowest offset
	for( j=0; j < Entries; ++j )
	{
		if( fe[j].Type )
			continue;

		if( fe[j].FileOffset <= CurrentOffset )
			CurrentOffset = fe[j].FileOffset;
	}

	CurrentOffset <<= 2;
	
	for( i=1; i < Entries; ++i )
	{
		if( level )
		{
			while( LEntry[level-1] == i )
			{
				level--;
			}
		}

		if( fe[i].Type )
		{
			//Skip empty folders
			if( fe[i].NextOffset == i+1 )
				continue;

			Entry[level] = i;
			LEntry[level++] = fe[i].NextOffset;
			if( level > 15 )	// something is wrong!
				break;

		} else {

			memset32( Path, 0, 256 );
			sprintf( Path, "%sfiles/", GamePath );

			for( j=0; j<level; ++j )
			{
				if( j )
					Path[strlen(Path)] = '/';
				memcpy( Path+strlen(Path), NameOff + fe[Entry[j]].NameOffset, strlen(NameOff + fe[Entry[j]].NameOffset ) );
			}
			if( level )
				Path[strlen(Path)] = '/';
			memcpy( Path+strlen(Path), NameOff + fe[i].NameOffset, strlen(NameOff + fe[i].NameOffset) );

			Asciify( Path );

			s32 fd = DVDOpen( Path, DREAD );
			if( fd < 0 )
			{
				dbgprintf( DEBUG_ERROR, "DIP:Couldn't open:\"%s\"\n", Path );				
				return 0;
			}
			
			fe[i].FileLength = DVDGetSize(fd);

			if( CurrentOffset == 0 )
				CurrentOffset = fe[i].FileOffset << 2;

			fe[i].FileOffset = CurrentOffset >> 2;

			DVDClose( fd );

			CurrentOffset += (fe[i].FileLength + 31) & (~31);	//align by 32 bytes

			dbgprintf( DEBUG_INFO, "\rDIP:%u/%u ...", i, Entries );
			//dbgprintf( DEBUG_INFO, "%s Size:%08X Offset:0x%X%08X\n", Path, fe[i].FileLength, (fe[i].FileOffset>>30), (u32)(fe[i].FileOffset<<2) );
		}
	}

	return 1;
}
s32 DVDLowRead( u32 Offset, u32 Length, void *ptr )
{
	s32 fd;
	u32 DebuggerHook=0;
	char Path[256];

	//dbgprintf("DIP:Read Offset:0x%08X Size:0x%08X as:%08X\n", Offset<<2, Length, ApploaderSize<<2 );

	if( Offset < 0x110 )	// 0x440
	{
		sprintf( Path, "%ssys/boot.bin", GamePath );
		fd = DVDOpen( Path, DREAD );
		if( fd < 0 )
		{
			dbgprintf( DEBUG_ERROR, "DIP:[boot.bin] Failed to open!\n" );
			return DI_FATAL;
		} else {
			dbgprintf( DEBUG_INFO, "DIP:[boot.bin] Offset:%08X Size:%08X\n", Offset<<2, Length );

			u32 *rbuf = (u32*)halloca( sizeof(u32), 32 );

			//read requested data
			DVDSeek( fd, Offset<<2, 0 );
			DVDRead( fd, ptr, Length );

			//Read DOL/FST offset/sizes for later usage
			DVDSeek( fd, 0x0420, 0 );
			DVDRead( fd, rbuf, 4 );
			DolOffset = *rbuf;

			DVDSeek( fd, 0x0424, 0 );
			DVDRead( fd, rbuf, 4 );
			FSTableOffset = *rbuf;

			DVDSeek( fd, 0x0428, 0 );
			DVDRead( fd, rbuf, 4 );
			FSTableSize = *rbuf;

			hfree( rbuf );

			DolSize = FSTableOffset - DolOffset;
			FSTableSize <<= 2;

			if( DiscType == DISC_REV )
			{
				dbgprintf( DEBUG_INFO, "DIP:FSTableOffset:%08X\n", FSTableOffset<<2 );
				dbgprintf( DEBUG_INFO, "DIP:FSTableSize:  %08X\n", FSTableSize );
				dbgprintf( DEBUG_INFO, "DIP:DolOffset:    %08X\n", DolOffset<<2 );
				dbgprintf( DEBUG_INFO, "DIP:DolSize:      %08X\n", DolSize<<2 );
			} else {
				dbgprintf( DEBUG_INFO, "DIP:FSTableOffset:%08X\n", FSTableOffset );
				dbgprintf( DEBUG_INFO, "DIP:FSTableSize:  %08X\n", FSTableSize );
				dbgprintf( DEBUG_INFO, "DIP:DolOffset:    %08X\n", DolOffset );
				dbgprintf( DEBUG_INFO, "DIP:DolSize:      %08X\n", DolSize );
			}

			DVDClose( fd );
			return DI_SUCCESS;
		}

	} else if( Offset < 0x910 )	// 0x2440
	{
		Offset -= 0x110;

		sprintf( Path, "%ssys/bi2.bin", GamePath );
		fd = DVDOpen( Path, DREAD );
		if( fd < 0 )
		{
			dbgprintf( DEBUG_ERROR, "DIP:[bi2.bin] Failed to open!\n" );
			return DI_FATAL;
		} else {
			dbgprintf( DEBUG_INFO, "DIP:[bi2.bin] Offset:%08X Size:%08X Dst:%p\n", Offset<<2, Length, ptr );
			DVDSeek( fd, Offset<<2, 0 );
			DVDRead( fd, ptr, Length );
			DVDClose( fd );

			//GC region patch
			if( DiscType == DISC_DOL )
			{
				*(vu32*)(ptr+0x18) = DICfg->Region;
				dbgprintf( DEBUG_INFO, "DIP:Patched GC region to:%d\n", DICfg->Region );
			}
			return DI_SUCCESS;
		}


	} else if( Offset < 0x910+ApploaderSize )	// 0x2440
	{
		Offset -= 0x910;

		sprintf( Path, "%ssys/apploader.img", GamePath );
		fd = DVDOpen( Path, DREAD );
		if( fd < 0 )
		{
			dbgprintf( DEBUG_ERROR, "DIP:[apploader.img] Failed to open!\n" );
			return DI_FATAL;
		} else {
			dbgprintf( DEBUG_INFO, "DIP:[apploader.img] Offset:%08X Size:%08X Dst:%p\n", Offset<<2, Length, ptr );
			DVDSeek( fd, Offset<<2, 0 );
			DVDRead( fd, ptr, Length );
			DVDClose( fd );
			return DI_SUCCESS;
		}
	} else if( Offset < DolOffset + DolSize )
	{
		Offset -= DolOffset;

		sprintf( Path, "%ssys/main.dol", GamePath );
		fd = DVDOpen( Path, DREAD );
		if( fd < 0 )
		{
			dbgprintf( DEBUG_ERROR, "DIP:[main.dol] Failed to open!\n" );
			return DI_FATAL;
		} else {
			dbgprintf( DEBUG_INFO, "DIP:[main.dol] Offset:%08X Size:%08X Dst:%p\n", Offset<<2, Length, ptr );
			DVDSeek( fd, Offset<<2, 0 );
			DVDRead( fd, ptr, Length );
			DVDClose( fd );
			
			int i;
			for( i=0; i < Length; i+=4 )
			{
				//Remote debugging and USBgecko debug printf doesn't work well at the same time!
				if( DICfg->Config & CONFIG_DEBUG_GAME )
				{
					switch( DICfg->Config&HOOK_TYPE_MASK )
					{
						case HOOK_TYPE_VSYNC:	
						{	
							//VIWaitForRetrace(Pattern 1)
							if( *(vu32*)(ptr+i) == 0x4182FFF0 && *(vu32*)(ptr+i+4) == 0x7FE3FB78 )
							{
								DebuggerHook = (u32)ptr + i + 8 * sizeof(u32);
								dbgprintf( DEBUG_INFO, "DIP:[patcher] Found VIWaitForRetrace pattern:%08X\n",  (u32)(ptr+i) | 0x80000000 );
							}
						} break;
						case HOOK_TYPE_OSLEEP:	
						{	
							//OSSleepThread(Pattern 1)
							if( *(vu32*)(ptr+i+0) == 0x3C808000 &&
								*(vu32*)(ptr+i+4) == 0x38000004 &&
								*(vu32*)(ptr+i+8) == 0x808400E4 )
							{
								int j = 0;

								while( *(vu32*)(ptr+i+j) != 0x4E800020 )
									j+=4;

								DebuggerHook = (u32)ptr + i + j;
								dbgprintf( DEBUG_INFO, "DIP:[patcher] Found OSSleepThread pattern:%08X\n",  (u32)(ptr+i) | 0x80000000 );
							}
						} break;
					}
					
				} else if( (DICfg->Config & CONFIG_PATCH_FWRITE) )
				{
					if( memcmp( (void*)(ptr+i), sig_fwrite, sizeof(sig_fwrite) ) == 0 )
					{
						dbgprintf( DEBUG_INFO, "DIP:[patcher] Found __fwrite pattern:%08X\n",  (u32)(ptr+i) | 0x80000000 );
						memcpy( (void*)(ptr+i), patch_fwrite, sizeof(patch_fwrite) );
					}
				}
				if( DICfg->Config & CONFIG_PATCH_MPVIDEO )
				{
					if( memcmp( (void*)(ptr+i), sig_iplmovie, sizeof(sig_iplmovie) ) == 0 )
					{
						dbgprintf( DEBUG_INFO, "DIP:[patcher] Found sig_iplmovie pattern:%08X\n", (u32)(ptr+i) | 0x80000000 );
						memcpy( (void*)(ptr+i), patch_iplmovie, sizeof(patch_iplmovie) );
					}
				}
				if( DICfg->Config & CONFIG_PATCH_VIDEO )
				{
					if( *(vu32*)(ptr+i) == 0x3C608000 )
					{
						if( ((*(vu32*)(ptr+i+4) & 0xFC1FFFFF ) == 0x800300CC) && ((*(vu32*)(ptr+i+8) >> 24) == 0x54 ) )
						{
							dbgprintf( DEBUG_INFO, "DIP:[patcher] Found VI pattern:%08X\n", (u32)(ptr+i) | 0x80000000 );
							*(vu32*)(ptr+i+4) = 0x5400F0BE | ((*(vu32*)(ptr+i+4) & 0x3E00000) >> 5	);
						}
					}
				}
			}

			if( GameHook != 0xdeadbeef )
			if( DebuggerHook && (DICfg->Config & CONFIG_DEBUG_GAME) )
			{
				GameHook = 0xdeadbeef;

				sprintf( Path, "/sneek/kenobiwii.bin" );
				s32 fd = IOS_Open( Path, 1 );
				if( fd < 0 )
				{
					dbgprintf( DEBUG_WARNING, "DIP:Could not open:\"%s\", this file is required for debugging!\n", Path );
					return DI_SUCCESS;
				}

				u32 Size = IOS_Seek( fd, 0, 2 );
				IOS_Seek( fd, 0, 0 );

				//Read file to memory
				s32 ret = IOS_Read( fd, (void*)0x1800, Size );
				if( ret != Size )
				{
					dbgprintf( DEBUG_WARNING, "DIP:Could not read:\"%s\":%d\n", Path, ret );
					IOS_Close( fd );
					return DI_SUCCESS;
				}
				IOS_Close( fd );

				*(vu32*)((*(vu32*)0x1808)&0x7FFFFFFF) = !!(DICfg->Config & CONFIG_DEBUG_GAME_WAIT);

				memcpy( (void *)0x1800, (void*)0, 6 );
				
				dbgprintf( DEBUG_INFO, "DIP:Debugger wait:%d\n", *(vu32*)((*(vu32*)0x1808)&0x7FFFFFFF) );

				u32 newval = 0x00018A8 - DebuggerHook;
					newval&= 0x03FFFFFC;
					newval|= 0x48000000;

				*(vu32*)(DebuggerHook) = newval;

				dbgprintf( DEBUG_INFO, "DIP:Hook@%08X(%08X)\n", DebuggerHook | 0x80000000, *(vu32*)(DebuggerHook) );

				//Begin section to add codes file
				sprintf( Path, "%scodes.gct", GamePath );
				s32 fc = DVDOpen( Path, DREAD );
				if ( fc < 0)
				{
					return DI_SUCCESS;
				}

				DVDRead( fc, (void*)0x27D0, DVDGetSize( fc ) );
				DVDClose( fc );
				*(vu8*)0x1807 = 0x01;
				//End section to add codes file

			}

			return DI_SUCCESS;
		}
	} else if( Offset < FSTableOffset+(FSTableSize>>2) )
	{
		Offset -= FSTableOffset;
		
		sprintf( Path, "%ssys/fst.bin", GamePath );
		fd = DVDOpen( Path, DREAD );
		if( fd < 0 )
		{
			dbgprintf( DEBUG_ERROR, "DIP:[fst.bin]  Failed to open!\n" );
			return DI_FATAL;
		} else {
			dbgprintf( DEBUG_INFO, "DIP:[fst.bin]  Offset:%08X Size:%08X Dst:%p\n", Offset, Length, ptr );
			DVDSeek( fd, (u64)Offset<<2, 0 );
			DVDRead( fd, ptr, Length );
			DVDClose( fd );

			if( FSTable == NULL )
			{
				FSTable	= (u8*)ptr;
				//dbgprintf("DIP:FSTOffset:  %08X\n", (u32)FSTable );
			}

			return DI_SUCCESS;
		}
	} else {
		
		//Get FSTTable offset from low memory, must be set by apploader
		if( FSTable == NULL )
		{
			FSTable	= (u8*)((*(vu32*)0x38) & 0x7FFFFFFF);
			//dbgprintf("DIP:FSTOffset:  %08X\n", (u32)FSTable );
		}

		u32 i,j;

		//try cache first!
		for( i=0; i < FILECACHE_MAX; ++i )
		{
			if( FC[i].File == 0xdeadbeef )
				continue;

			if( Offset >= FC[i].Offset )
			{
				u64 nOffset = ((u64)(Offset-FC[i].Offset)) << 2;
				if( nOffset < FC[i].Size )
				{
					//dbgprintf("DIP:[Cache:%02d][%08X:%05X]\n", i, (u32)(nOffset>>2), Length );
					DVDSeek( FC[i].File, nOffset, 0 );
					DVDRead( FC[i].File, ptr, ((Length)+31)&(~31) );
					return DI_SUCCESS;
				}
			}
		}

		//The fun part!

		u32 Entries = *(u32*)(FSTable+0x08);
		char *NameOff = (char*)(FSTable + Entries * 0x0C);
		FEntry *fe = (FEntry*)(FSTable);

		u32 Entry[16];
		u32 LEntry[16];
		u32 level=0;

		for( i=1; i < Entries; ++i )
		{
			if( level )
			{
				while( LEntry[level-1] == i )
				{
					//printf("[%03X]leaving :\"%s\" Level:%d\n", i, buffer + NameOff + swap24( fe[Entry[level-1]].NameOffset ), level );
					level--;
				}
			}

			if( fe[i].Type )
			{
				//Skip empty folders
				if( fe[i].NextOffset == i+1 )
					continue;

				//printf("[%03X]Entering:\"%s\" Level:%d leave:%04X\n", i, buffer + NameOff + swap24( fe[i].NameOffset ), level, swap32( fe[i].NextOffset ) );
				Entry[level] = i;
				LEntry[level++] = fe[i].NextOffset;
				if( level > 15 )	// something is wrong!
					break;
			} else {

				if( Offset >= fe[i].FileOffset )
				{
					u64 nOffset = ((u64)(Offset-fe[i].FileOffset)) << 2;
					if( nOffset < fe[i].FileLength )
					{
						//dbgprintf("DIP:Offset:%08X FOffset:%08X Dif:%08X Flen:%08X nOffset:%08X\n", Offset, fe[i].FileOffset, Offset-fe[i].FileOffset, fe[i].FileLength, nOffset );

						//dbgprintf("Found Entry:%X file:%s FCEntry:%d\n", i,  FSTable + NameOff + fe[i].NameOffset, FCEntry );

						//Do not remove!
						memset( Path, 0, 256 );					
						sprintf( Path, "%sfiles/", GamePath );

						for( j=0; j<level; ++j )
						{
							if( j )
								Path[strlen(Path)] = '/';
							memcpy( Path+strlen(Path), NameOff + fe[Entry[j]].NameOffset, strlen(NameOff + fe[Entry[j]].NameOffset ) );
						}
						if( level )
							Path[strlen(Path)] = '/';
						memcpy( Path+strlen(Path), NameOff + fe[i].NameOffset, strlen(NameOff + fe[i].NameOffset) );

						if( FCEntry >= FILECACHE_MAX )
							FCEntry = 0;

						if( FC[FCEntry].File != 0xdeadbeef )
						{
							DVDClose( FC[FCEntry].File );
							FC[FCEntry].File = 0xdeadbeef;
						}

						Asciify( Path );

						FC[FCEntry].File = DVDOpen( Path, DREAD );
						if( FC[FCEntry].File < 0 )
						{
							FC[FCEntry].File = 0xdeadbeef;

							//dbgprintf( DEBUG_ERROR, "DIP:[%s] Failed to open!\n", Path );
							error = 0x031100;
							return DI_FATAL;
						} else {

							FC[FCEntry].Size	= fe[i].FileLength;
							FC[FCEntry].Offset	= fe[i].FileOffset;

							//Don't output anything while debugging!
							if( !(DICfg->Config & CONFIG_DEBUG_GAME) )
								dbgprintf( DEBUG_INFO, "DIP:[%s][%08X:%05X]\n", Path+7, (u32)(nOffset>>2), Length );	// +7 skips the "/games/" part

							DVDSeek( FC[FCEntry].File, nOffset, 0 );
							DVDRead( FC[FCEntry].File, ptr, Length );
							
							FCEntry++;
								
							if( DICfg->Config & ( CONFIG_FST_REBUILD_TEMP | CONFIG_FST_REBUILD_PERMA ) )
							{
								//After the game read the opening.bnr we rebuild the FST
								if( strstr( Path, "opening.bnr" ) != NULL )
								{
									if( FSTRebuild() )
									{
										if( DICfg->Config & CONFIG_FST_REBUILD_PERMA )
										{
											sprintf( Path, "%ssys/fst.bin", GamePath );
											fd = DVDOpen( Path, DWRITE );
											if( fd >= 0 )
											{
												if( DVDWrite( fd, (void*)FSTable, FSTableSize ) != FSTableSize )
												{
													dbgprintf( DEBUG_INFO, "DIP:Failed to write:%p %u\n", FSTable, FSTableSize );														
												}
												DVDClose( fd );
											} else {
												dbgprintf( DEBUG_INFO, "DIP:Failed to open:\"%s\":%d\n", Path, fd );
											}
										}
									} else {
										DICover |= 1;	// set to no disc an half modified FST will just cause trouble
									}

									DICfg->Config &= ~(CONFIG_FST_REBUILD_PERMA|CONFIG_FST_REBUILD_TEMP);
									DICfgFlush( DICfg );
								}
							}
							return DI_SUCCESS;
						}
					}
				}
			}
		}
	}
	return DI_FATAL;
}
s32 DVDLowReadUnencrypted( u32 Offset, u32 Length, void *ptr )
{
	switch( Offset )
	{
		case 0x00:
		case 0x08:		// 0x20
		{
			char *str = (char *)malloca( 64, 32 );
			sprintf( str, "%ssys/boot.bin", GamePath );

			u32 ret = DI_FATAL;
			s32 fd = DVDOpen( str, FA_READ );
			if( fd >= 0 )
			{
				if( DVDRead( fd, ptr, Length ) == Length )
				{
					free( str );
					ret = DI_SUCCESS;				
				}
				DVDClose( fd );
			}

			free( str );
			return ret;
			
		} break;
		case 0x10000:		// 0x40000
		{
			memset( ptr, 0, Length );

			*(u32*)(ptr)	= 1;				// one partition
			*(u32*)(ptr+4)	= 0x40020>>2;		// partition table info

			return DI_SUCCESS;
		} break;
		case 0x10008:		// 0x40020
		{
			memset( ptr, 0, Length );

			*(u32*)(ptr)	= 0x03E00000;		// partition offset
			*(u32*)(ptr+4)	= 0x00000000;		// partition type

			return DI_SUCCESS;
		} break;
		case 0x00013800:		// 0x4E000
		{
			memset( ptr, 0, Length );

			*(u32*)(ptr)		= DICfg->Region;
			*(u32*)(ptr+0x1FFC)	= 0xC3F81A8E;

			dbgprintf( DEBUG_DEBUG, "DIP:Region:%d\n", DICfg->Region );

			return DI_SUCCESS;
		} break;
		default:
			//dbgprintf("DIP:unexpected read offset!\n");
			//dbgprintf("DIP:DVDLowUnencryptedRead( %08X, %08X, %p )\n", Offset, Length, ptr );
			while(1);
		break;
	}
}
s32 DVDLowReadDiscID( u32 Offset, u32 Length, void *ptr )
{
	char *str = (char *)malloca( 256, 32 );
	sprintf( str, "%ssys/boot.bin", GamePath );
	s32 fd = DVDOpen( str, FA_READ );
	if( fd < 0 )
	{
		return DI_FATAL;
	} else {
		DVDRead( fd, ptr, Length );
		DVDClose( fd );
	}

	free( str );

#ifdef DEBUG
	hexdump( ptr, Length );
#endif

	if( *(vu32*)(ptr+0x1C) == 0xc2339f3d )
	{
		DiscType = DISC_DOL;
	} else if( *(vu32*)(ptr+0x18) == 0x5D1C9EA3 )
	{
		DiscType = DISC_REV;
	} else {
		DiscType = DISC_INV;
	}
	
	return DI_SUCCESS;
}

void DIP_Fatal( char *name, u32 line, char *file, s32 error, char *msg )
{
	//dbgprintf("************ DI FATAL ERROR ************\n");
	//dbgprintf("Function :%s\n", name );
	//dbgprintf("line     :%d\n", line );
	//dbgprintf("file     :%s\n", file );
	//dbgprintf("error    :%d\n", error );
	//dbgprintf("%s\n", msg );
	//dbgprintf("************ DI FATAL ERROR ************\n");

	while(1);
}
int DIP_Ioctl( struct ipcmessage *msg )
{
	u8  *bufin  = (u8*)msg->ioctl.buffer_in;
	u32 lenin   = msg->ioctl.length_in;
	u8  *bufout = (u8*)msg->ioctl.buffer_io;
	u32 lenout  = msg->ioctl.length_io;
	s32 ret		= DI_FATAL;
	s32 fd;

	//dbgprintf("DIP:IOS_Ioctl( %d 0x%x 0x%p 0x%x 0x%p 0x%x )\n", msg->fd, msg->ioctl.command, bufin, lenin, bufout, lenout);
			
	switch(msg->ioctl.command)
	{
		//Added for slow harddrives
		case DVD_CONNECTED:
		{
			if( HardDriveConnected )
				ret = 1;
			else
				ret = 0;

			dbgprintf( DEBUG_DEBUG, "DIP:DVDConnected():%d\n", ret );
		} break;
		case DVD_WRITE_CONFIG:
		{
			u32 *vec = (u32*)msg->ioctl.buffer_in;
			char *name = (char*)halloca( 256, 32 );
			
			memcpy( DICfg, (u8*)(vec[0]), DVD_CONFIG_SIZE );

			sprintf( name, "%s", "/sneek/diconfig.bin" );
			fd = DVDOpen( name, FA_WRITE|FA_OPEN_EXISTING );
			if( fd < 0 )
			{
				DVDDelete( name );
				fd = DVDOpen( name, FA_WRITE|FA_CREATE_ALWAYS );
				if( fd < 0 )
				{
					ret = DI_FATAL;
					break;
				}
			}

			DVDWrite( fd, DICfg, DVD_CONFIG_SIZE );
			DVDClose( fd );					

			dbgprintf( DEBUG_DEBUG, "DIP:Region:%d SlotID:%d GameCount:%d Config:%04X\n", ((DIConfig*)(vec[0]))->Region, ((DIConfig*)(vec[0]))->SlotID, ((DIConfig*)(vec[0]))->Gamecount, ((DIConfig*)(vec[0]))->Config  );

			ret = DI_SUCCESS;
			hfree( name );
			hfree( name );

			dbgprintf( DEBUG_DEBUG, "DIP:DVDWriteDIConfig( %p ):%d\n", vec[0], ret );
		} break;
		case DVD_READ_GAMEINFO:
		{
			u32 *vec = (u32*)msg->ioctl.buffer_in;

			fd = DVDOpen( "/sneek/diconfig.bin", FA_READ );
			if( fd < 0 )
			{
				ret = DI_FATAL;
			} else {

				DVDSeek( fd, vec[0], 0 );
				DVDRead( fd, (u8*)(vec[2]), vec[1] );
				DVDClose( fd );

				ret = DI_SUCCESS;
			}

			dbgprintf( DEBUG_DEBUG, "DIP:DVDReadGameInfo( %08X, %08X, %p ):%d\n", vec[0], vec[1], vec[2], ret );
		} break;
		case DVD_INSERT_DISC:
		{
			DICover &= ~1;
			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:InsertDisc(%d):%d\n", ret );
		} break;
		case DVD_EJECT_DISC:
		{
			DICover |= 1;
			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:EjectDisc():%d\n", ret );
		} break;
		case DVD_GET_GAMECOUNT:	//Get Game count
		{
			*(u32*)bufout = DICfg->Gamecount;
			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:GetGameCount(%d):%d\n", DICfg->Gamecount, ret );
		} break;
		case DVD_SELECT_GAME:
		{
			ret = DVDSelectGame( *(u32*)(bufin) );

			dbgprintf( DEBUG_DEBUG, "DIP:DVDSelectGame(%d):%d\n", *(u32*)(bufin), ret );
		} break;
		case DVD_SET_AUDIO_BUFFER:
		{
			//hexdump( bufin, lenin );
			memset32( bufout, 0, lenout );

			static u32 isASEnabled = 0;
			
			if( isASEnabled == 0 )
			{
				write32( 0x0D806008, 0xA8000040 );
				write32( 0x0D80600C, 0 );
				write32( 0x0D806010, 0x20 );
				write32( 0x0D806018, 0x20 );
	
				write32( 0x0D806014, (u32)0 );

				write32( 0x0D806000, 0x3A );
	
				write32( 0x0D80601C, 3 );		// send cmd
				while( (read32( 0x0D806000 ) & 0x14) == 0 );

				dbgprintf( DEBUG_DEBUG, "DIP:RealLowReadDiscID(%02X)\n", read32( 0x0D806000 ) & 0x54 );

				write32( 0x0D806004, read32( 0x0D806004 ) );

				write32( 0x0D806008, 0xE4000000 | 0x10000 | 0x0A );
	
				write32( 0x0D80601C, 1 );

				while( read32(0x0D80601C) & 1 );
			
				dbgprintf( DEBUG_DEBUG, "DIP:RealEnableAudioStreaming(%02X)\n", read32( 0x0D806000 ) & 0x54 );

				isASEnabled = 1;
			}

			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowConfigAudioBuffer():%d\n", ret );
		} break;
		case 0x96:
		case DVD_REPORTKEY:
		{
			ret = DI_ERROR;
			error = 0x00052000;
			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowReportKey():%d\n", ret );
		} break;
		case 0xDD:			// 0 out
		{
			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowSetMaximumRotation():%d\n", ret);
		} break;
		case 0x95:			// 0x20 out
		{
			*(u32*)bufout = DIStatus;

			ret = DI_SUCCESS;
			//dbgprintf( DEBUG_DEBUG, "DIP:DVDLowPrepareStatusRegister( %08X ):%d\n", *(u32*)bufout, ret );
		} break;
		case 0x7A:			// 0x20 out
		{
			*(u32*)bufout = DICover;

			if( ChangeDisc )
			{
				DICover &= ~1;
				ChangeDisc = 0;
			}

			ret = DI_SUCCESS;
			//dbgprintf( DEBUG_DEBUG, "DIP:DVDLowPrepareCoverRegister( %08X ):%d\n", *(u32*)bufout, ret );
		} break;
		case 0x86:			// 0 out
		{
			ret = DI_SUCCESS;
			//dbgprintf( DEBUG_DEBUG, "DIP:DVDLowClearCoverInterrupt():%d\n", ret);
		} break;
		case DVD_GETCOVER:	// 0x88
		{
			*(u32*)bufout = DICover;
			ret = DI_SUCCESS;
			//dbgprintf( DEBUG_DEBUG, "DIP:DVDLowGetCoverStatus( %08X ):%d\n", *(u32*)bufout, ret );
		} break;
		case 0x89:
		{
			DICover &= ~4;
			DICover |= 2;

			ret = DI_SUCCESS;
			//dbgprintf( DEBUG_DEBUG, "DIP:DVDLowUnknownRegister():%d\n", ret );
		} break;
		case DVD_IDENTIFY:
		{
			memset( bufout, 0, 0x20 );

			*(u32*)(bufout)		= 0x00000002;
			*(u32*)(bufout+4)	= 0x20070213;
			*(u32*)(bufout+8)	= 0x41000000;

			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowIdentify():%d\n", ret);
		} break;
		case DVD_GET_ERROR:	// 0xE0
		{
			*(u32*)bufout = error; 
			ret = DI_SUCCESS;
			dbgprintf( DEBUG_ERROR, "DIP:DVDLowGetError( %08X ):%d\n", error, ret );
		} break;
		case 0x8E:
		{
			if( (*(u8*)(bufin+4)) == 0 )
				EnableVideo( 1 );
			else
				EnableVideo( 0 );

			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowEnableVideo(%d):%d\n", *(u32*)(bufin+4), ret);
		} break;
		case DVD_LOW_READ:	// 0x71, GC never calls this
		{
			if( Partition )
			{
				if( *(u32*)(bufin+8) > PartitionSize )
				{
					ret = DI_ERROR;
					error = 0x00052100;
				} else {
					ret = DVDLowRead( *(u32*)(bufin+8), *(u32*)(bufin+4), bufout );
				}
			} else {
				;//DIP_Fatal("DVDLowRead", __LINE__, __FILE__, 64, "Partition not opened!");
			}
			if( ret < 0 )
				dbgprintf( DEBUG_DEBUG, "DIP:DVDLowRead( %08X, %08X, %p ):%d\n", *(u32*)(bufin+8), *(u32*)(bufin+4), bufout, ret );
		} break;
		case DVD_READ_UNENCRYPTED:
		{
			if( DMLite )
				FSMode = UNEEK;

			if( DiscType == DISC_DOL )
			{
				ret = DVDLowRead( *(u32*)(bufin+8), *(u32*)(bufin+4), bufout );

			} else if(  DiscType == DISC_REV )
			{
				if( *(u32*)(bufin+8) > 0x46090000 )
				{
					error = 0x00052100;
					return DI_ERROR;

				} else {
					ret = DVDLowReadUnencrypted( *(u32*)(bufin+8), *(u32*)(bufin+4), bufout );
				}
			} else {
				//Invalid disc type!
				ret = DI_FATAL;
			}

			if( DMLite )
				FSMode = SNEEK;

			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowUnencryptedRead( %08X, %08X, %p ):%d\n", *(u32*)(bufin+8), *(u32*)(bufin+4), bufout, ret );
		} break;
		case DVD_READ_DISCID:
		{
			if( DMLite )
				FSMode = UNEEK;

			ret = DVDLowReadDiscID( 0, lenout, bufout );

			if( DMLite )
				FSMode = SNEEK;

			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowReadDiscID(%p):%d\n", bufout, ret );
		} break;
		case DVD_RESET:
		{
			DIStatus = 0x00;
			DICover  = 0x00;
			Partition = 0;

			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowReset( %d ):%d\n", *(u32*)(bufin+4), ret);
		} break;
		case DVD_SET_MOTOR:
		{
			Motor = 0;
			ret = DI_SUCCESS;
			dbgprintf( DEBUG_ERROR, "DIP:DVDStopMotor(%d,%d):%d\n", *(u32*)(bufin+4), *(u32*)(bufin+8), ret);
		} break;
		case DVD_CLOSE_PARTITION:
		{
			Partition=0;
			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowClosePartition():%d\n", ret );
		} break;
		case DVD_READ_BCA:
		{
			memset32( (u32*)bufout, 0, 0x40 );
			*(u32*)(bufout+0x30) = 0x00000001;

			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowReadBCA():%d\n", ret );
		} break;
		case DVD_LOW_SEEK:
		{
			ret = DI_SUCCESS;
			dbgprintf( DEBUG_DEBUG, "DIP:DVDLowSeek():%d\n", ret );
		} break;
		default:
			hexdump( bufin, lenin );
			//dbgprintf( DEBUG_ERROR, "DIP:Unknown IOS_Ioctl( %d 0x%x 0x%p 0x%x 0x%p 0x%x )\n", msg->fd, msg->ioctl.command, bufin, lenin, bufout, lenout );
			MessageQueueAck( (struct ipcmessage *)msg, DI_FATAL);
			while(1);
		break;
	}

	//Reset errror after a successful command
	if( msg->command != 0xE0 && ret == DI_SUCCESS )
		error = 0;

	return ret;
}
int DIP_Ioctlv(struct ipcmessage *msg)
{
	vector *v	= (vector*)(msg->ioctlv.argv);
	s32 ret		= DI_FATAL;

	//dbgprintf("DIP:IOS_Ioctlv( %d, 0x%x 0x%x 0x%x 0x%p )\n", msg->fd, msg->ioctlv.command, msg->ioctlv.argc_in, msg->ioctlv.argc_io, msg->ioctlv.argv );
	switch(msg->ioctl.command)
	{
		case DVD_CREATEDIR:
		{
			ret = DVDCreateDir( (char*)(v[0].data) );
		} break;
		case DVD_CLOSE:
		{
			DVDClose( (u32)(v[0].data) );
			ret = DI_SUCCESS;
		} break;
		case DVD_WRITE:
		{
			//dbgprintf("DVDWrite( %d, %p, %d)\n", (u32)(v[0].data), (u8*)(v[1].data), v[1].len );
			ret = DVDWrite( (u32)(v[0].data), (u8*)(v[1].data), v[1].len );
		} break;
		case DVD_READ:
		{
			ret = DVDRead( (u32)(v[0].data), (u8*)(v[1].data), v[1].len );
		} break;
		case DVD_OPEN:
		{
			s32 fd = DVDOpen( (char*)(v[0].data), FA_WRITE|FA_READ|FA_CREATE_ALWAYS );
			if( fd < 0 )
			{
				ret = DI_FATAL;
				break;
			}
			ret = fd;
		} break;
		case DVD_OPEN_PARTITION:
		{
			//if( Partition == 1 )
			//{
			//	DIP_Fatal( "DVDLowOpenPartition", __LINE__, __FILE__, 0, "Partition already open!");
			//}

			PartitionOffset = *(u32*)(v[0].data+4);
			PartitionOffset <<= 2;

			u8 *TMD;
			u8 *TIK;
			u8 *CRT;
			u8 *hashes = (u8*)halloca( sizeof(u8)*0x14, 32 );
			u8 *buffer = (u8*)halloca( 0x40, 32 );
			memset( buffer, 0, 0x40 );

			char *str = (char*)halloca( 0x40,32 );

			sprintf( str, "%sticket.bin", GamePath );

			s32 fd = DVDOpen( str, FA_READ );
			if( fd < 0 )
			{
				//dbgprintf("DIP:Could not open:\"%s\"\n", str );
				ret = DI_FATAL;
				break;
			} else {
				TIK = (u8*)halloca( 0x2a4, 32 );

				ret = DVDRead( fd, TIK, 0x2a4 );

				if( ret != 0x2A4 )
				{
					ret = DI_FATAL;
					DVDClose( fd );
					//dbgprintf("DIP:Failed to read:\"%s\":%d\n", str, ret );
					break;
				}

				((u32*)buffer)[0x04] = (u32)TIK;			//0x10
				((u32*)buffer)[0x05] = 0x2a4;				//0x14
				sync_before_read(TIK, 0x2a4);
				
				DVDClose( fd );
			}

			sprintf( str, "%stmd.bin", GamePath );
			
			fd = DVDOpen( str, FA_READ );
			if( fd < 0 )
			{
				//dbgprintf("DIP:Could not open:\"%s\"\n", str );
				ret = DI_FATAL;
				break;
			} else {
				u32 asize = (DVDGetSize(fd)+31)&(~31);
				TMD = (u8*)halloca( asize, 32 );
				memset( TMD, 0, asize );
				ret = DVDRead( fd, TMD, DVDGetSize(fd) );
				if( ret != DVDGetSize(fd) )
				{
					ret = DI_FATAL;
					DVDClose( fd );
					//dbgprintf("DIP:Failed to read:\"%s\":%d\n", str, ret );
					break;
				} 

				((u32*)buffer)[0x06] = (u32)TMD;		//0x18
				((u32*)buffer)[0x07] = DVDGetSize(fd);		//0x1C
				sync_before_read(TMD, asize);

				PartitionSize = (u32)((*(u64*)(TMD+0x1EC)) >> 2 );
				//dbgprintf("DIP:Set partition size to:%02X%08X\n", (u32)((((u64)PartitionSize)<<2)>>32), (PartitionSize<<2) );

				if( v[3].data != NULL )
				{
					memcpy( v[3].data, TMD, DVDGetSize(fd) );
					sync_after_write( v[3].data, DVDGetSize(fd) );
				}

				DVDClose( fd );
			}

			sprintf( str, "%scert.bin", GamePath );
			fd = DVDOpen( str, FA_READ );
			if( fd < 0 )
			{
				//dbgprintf("DIP:Could not open:\"%s\"\n", str );
				ret = DI_FATAL;
				break;
			} else {
				CRT = (u8*)halloca( 0xA00, 32 );
				ret = DVDRead( fd, CRT, 0xA00 );
				if( ret != 0xA00 )
				{
					ret = DI_FATAL;
					DVDClose( fd );
					//dbgprintf("DIP:Failed to read:\"%s\":%d\n", str, ret );
					break;
				}

				((u32*)buffer)[0x00] = (u32)CRT;		//0x00
				((u32*)buffer)[0x01] = 0xA00;			//0x04
				sync_before_read(CRT, 0xA00);

				DVDClose( fd );
			}

			hfree( str );

			KeyID = (u32*)malloca( sizeof( u32 ), 32 );


			((u32*)buffer)[0x02] = (u32)NULL;			//0x08
			((u32*)buffer)[0x03] = 0;					//0x0C

			//out
			((u32*)buffer)[0x08] = (u32)KeyID;			//0x20
			((u32*)buffer)[0x09] = 4;					//0x24
			((u32*)buffer)[0x0A] = (u32)hashes;			//0x28
			((u32*)buffer)[0x0B] = 20;					//0x2C

			sync_before_read(buffer, 0x40);

			s32 ESHandle = IOS_Open("/dev/es", 0 );

			/*s32 r =*/ IOS_Ioctlv( ESHandle, 0x1C, 4, 2, buffer );
			//if( r < 0)
			//	dbgprintf("ios_ioctlv( %d, %02X, %d, %d, %p ):%d\n", ESHandle, 0x1C, 4, 2, buffer, r );

			IOS_Close( ESHandle );

			*(u32*)(v[4].data) = 0x00000000;

			free( KeyID );

			hfree( buffer );
			hfree( TIK );
			hfree( CRT );
			hfree( TMD );

			//if( r < 0 )
			//{
			//	DIP_Fatal( "DVDLowOpenPartition", __LINE__, __FILE__, r, "ios_ioctlv() failed!");
			//}
			
			ret = DI_SUCCESS;

			Partition=1;

			dbgprintf( DEBUG_DEBUG, "DIP:DVDOpenPartition(0x%08X):%d\n", *(u32*)(v[0].data+4), ret );

		} break;
		default:
			MessageQueueAck( (struct ipcmessage *)msg, DI_FATAL);
			while(1);
		break;
	}

	//Reset errror after a successful command
	if( ret == DI_SUCCESS )
		error = 0;

	return ret;
}
