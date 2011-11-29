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

#include "Config.h"

//#define CONFIG_DEBUG

u32 ConfigLoaded	= 0;
u8 *SettingsBuf		= (u8*)NULL;
u8 *SysconfBuf		= (u8*)NULL;
SysConfigHeader *SysConfHdr = (SysConfigHeader *)NULL;

char *AreaStr[] = 
{
	"JPN",
	"USA",
	"EUR",
	"KOR",
};

char *VideoStr[] = 
{
	"NTSC",
	"PAL",
	"MPAL",
};

char *GameRegionStr[] = 
{
	"JP",
	"US",
	"EU",
	"KR",
};

s32 Config_InitSetting( void )
{
	u32 i;

	s32 fd = IOS_Open("/title/00000001/00000002/data/setting.txt", 1 );

	if( fd < 0 )
		return fd;

	SettingsBuf = (u8*)malloca( 256, 32 );
	IOS_Read( fd, SettingsBuf, 256 );
	IOS_Close( fd );

	//Decrypt buffer
	u32 magic = 0x73B5DBFA;
	for( i=0; i < 256; ++i )
	{
		SettingsBuf[i] = SettingsBuf[i] ^ (magic&0xFF);
		magic = (magic<<1) | (magic>>31);
	}

	return 1;
}
s32 Config_InitSYSCONF( void )
{
	s32 fd = IOS_Open("/shared2/sys/SYSCONF", 1 );

	SysconfBuf = (u8*)malloca( 0x4000, 32 );

	IOS_Read( fd, SysconfBuf, 0x4000 );

	IOS_Close( fd );

	SysConfHdr = (SysConfigHeader*)SysconfBuf;
	
	return 1;
}
u32 Config_UpdateSysconf( void )
{
	s32 fd = IOS_Open("/shared2/sys/SYSCONF", 2 );

	IOS_Write( fd, SysconfBuf, 0x4000 );

	IOS_Close( fd );

	return 1;
}
u32 SCSetByte( char *Name, u8 Value )
{
	u32 i;

	for( i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), Name, strlen(Name) ) == 0 )
		{
			dbgprintf("ES:SCSetByte(\"%s\", %u) Current value:%u\n", Name, Value, *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + strlen(Name)) );
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + strlen(Name)) = Value;
			break;
		}
	}
	return 0;
	
}
char *Config_GetSettingsString( char *Name )
{
	char *s = strstr( (char*)SettingsBuf, Name );

	if( s == NULL )
		return (char *)NULL;

	s += strlen(Name)+1; //ie: "AREA="

	//get value
	u32 len=0;
	while( isascii(s[len]) && (u8)(s[len])>0x20 )
	{
		len++;
		if( len > 100 )
			return (char *)NULL;
	}


	if( len <= 0 )
	{
		return (char *)NULL;
	}

	char *c = (char *)malloca( len + 1, 32 );
	memcpy( c, s, len + 1 );
	c[len] = 0;

	return c;
}
u32 Config_GetSettingsValue( char *Name )
{
	char *s = Config_GetSettingsString( Name );
	u32 i;

	if( s == NULL )
	{
		return -1;
	}

	if( strcmp( Name, "AREA") == 0 )
	{
		for( i=0; i < 4; ++i )
		{
			if( strcmp( s, AreaStr[i] ) == 0 )
			{
				free( s );
				return i;
			}
		}

	} else if( strcmp( Name, "VIDEO") == 0 ) { 

		for( i=0; i < 3; ++i )
		{
			if( strcmp( s, VideoStr[i] ) == 0 )
			{
				free( s );
				return i;
			}
		}

	} else if( strcmp( Name, "GAME") == 0 ) { 

		for( i=0; i < 4; ++i )
		{
			if( strcmp( s, GameRegionStr[i] ) == 0 )
			{
				free( s );
				return i;
			}
		}
	}
	
	free( s );

	return -2;
}
s32 Config_SetSettingsValue( char *Name, u32 value )
{
	char	*area	= (char *)NULL,
			*model	= (char *)NULL,
			*dvd	= (char *)NULL,
			*mpch	= (char *)NULL,
			*code	= (char *)NULL,
			*serno	= (char *)NULL,
			*video	= (char *)NULL,
			*game	= (char *)NULL;

	model	= Config_GetSettingsString("MODEL");
	dvd		= Config_GetSettingsString("DVD");
	mpch	= Config_GetSettingsString("MPCH");
	code	= Config_GetSettingsString("CODE");
	serno	= Config_GetSettingsString("SERNO");

	if( strcmp( Name, "AREA" ) == 0 )
	{
		area = AreaStr[value];
		video = Config_GetSettingsString("VIDEO");
		game = Config_GetSettingsString("GAME");

	} else if( strcmp( Name, "VIDEO" ) == 0 )
	{
		area = Config_GetSettingsString("AREA");
		video = VideoStr[value];
		game = Config_GetSettingsString("GAME");

	} else if( strcmp( Name, "GAME" ) == 0 )
	{
		area = Config_GetSettingsString("AREA");
		video = Config_GetSettingsString("VIDEO");
		game = GameRegionStr[value];
	}
	memset32( SettingsBuf, 0, 256 );

	_sprintf( (char*)SettingsBuf, "AREA=%s\nMODEL=%s\nDVD=%s\nMPCH=%s\nCODE=%s\nSERNO=%s\nVIDEO=%s\nGAME=%s\n", area, model, dvd, mpch, code, serno, video, game );

	if( strcmp( Name, "AREA" ) == 0 )
	{
		free( game );
		free( video );

	} else if( strcmp( Name, "VIDEO" ) == 0 )
	{
		free( area );
		free( game );

	} else if( strcmp( Name, "GAME" ) == 0 )
	{
		free( area );
		free( video );
	}

	free( model );
	free( dvd );
	free( mpch );
	free( code );
	free( serno );

	return Config_SettingsFlush();
}
s32 Config_SettingsFlush( void )
{
	//Encrypt buffer

	u32 magic = 0x73B5DBFA;
	u32 i;

	for( i=0; i < 256; ++i )
	{
		SettingsBuf[i] = SettingsBuf[i] ^ (magic&0xFF);
		magic = (magic<<1) | (magic>>31);
	}

	s32 fd = IOS_Open("/title/00000001/00000002/data/setting.txt", 2 );
	IOS_Write( fd, SettingsBuf, 256 );
	IOS_Close( fd );

	free( SysconfBuf );
	Config_InitSetting();

	return 1;
}
void Config_ChangeSystem( u64 TitleID, u16 TitleVersion )
{
	Config_InitSYSCONF();
	Config_InitSetting();

	if( TitleID == 0x0000000100000002LL ) 
	{
		dbgprintf("ES:Setting console to %s\n", AreaStr[TitleVersion & 0xF] );

		switch( TitleVersion & 0xF )
		{
			case AREA_JAP:
			{
				SCSetByte("IPL.E60", 0 );
				SCSetByte("IPL.PGS", 1 );
				SCSetByte("IPL.LNG", Japanse );
				
				Config_SetSettingsValue( "AREA", AREA_JAP );
				Config_SetSettingsValue( "VIDEO", NTSC );
				Config_SetSettingsValue( "GAME", JP );
				
			} break;
			case AREA_USA:
			{
				SCSetByte("IPL.E60", 0 );
				SCSetByte("IPL.PGS", 1 );
				SCSetByte("IPL.LNG", English );
				
				Config_SetSettingsValue( "AREA", AREA_USA );
				Config_SetSettingsValue( "VIDEO", NTSC );
				Config_SetSettingsValue( "GAME", US );
				
			} break;
			case AREA_EUR:
			{
				SCSetByte("IPL.E60", 1 );
				SCSetByte("IPL.PGS", 0 );
				SCSetByte("IPL.LNG", English );
				
				Config_SetSettingsValue( "AREA", AREA_EUR );
				Config_SetSettingsValue( "VIDEO", PAL );
				Config_SetSettingsValue( "GAME", EU );				
				
			} break;
		}
	} else {
		switch( TitleID & 0xFF )
		{
			case 'J':
			{
				dbgprintf("ES:Setting console to JAP\n");
				
				SCSetByte("IPL.E60", 0 );
				SCSetByte("IPL.PGS", 1 );
				SCSetByte("IPL.LNG", Japanse );
				
				Config_SetSettingsValue( "AREA", AREA_JAP );
				Config_SetSettingsValue( "VIDEO", NTSC );
				Config_SetSettingsValue( "GAME", JP );
				
			} break;
			case 'E':
			{
				dbgprintf("ES:Setting console to USA\n");
				
				SCSetByte("IPL.E60", 0 );
				SCSetByte("IPL.PGS", 1 );
				SCSetByte("IPL.LNG", English );
				
				Config_SetSettingsValue( "AREA", AREA_USA );
				Config_SetSettingsValue( "VIDEO", NTSC );
				Config_SetSettingsValue( "GAME", US );
				
			} break;
			case 'F':	// France
			case 'I':	// Italy
			case 'U':	// United Kingdom
			case 'S':	// Spain
			case 'D':	// Germany
			case 'P':
			{
				dbgprintf("ES:Setting console to EUR\n");
				
				SCSetByte("IPL.E60", 1 );
				SCSetByte("IPL.PGS", 0 );
				SCSetByte("IPL.LNG", English );
				
				Config_SetSettingsValue( "AREA", AREA_EUR );
				Config_SetSettingsValue( "VIDEO", PAL );
				Config_SetSettingsValue( "GAME", EU );
				
			} break;			
		}
	}

	Config_SettingsFlush();
	//Config_UpdateSysconf();
}
