#include "SMenu.h"


u32 FrameBuffer	= 0;
u32 FBOffset	= 0;
u32 FBEnable	= 0;
u32	FBSize		= 0;
u32 *WPad		= NULL;

u32 DVDReinsertDisc=false;

u32 ShowMenu=0;
u32 MenuType=0;
u32 SLock=0;
s32 PosX=0,ScrollX=0;

u32 *FB;

u32 Freeze;
u32 value;
u32 *offset;

extern u32 FSUSB;

u32 PosValX;
u32 Hits;
u32 edit;
u32 *Offsets;

GCPadStatus GCPad;

DIConfig *DICfg;

ChannelCache* channelCache;

ImageStruct* curDVDCover = NULL;

char *PICBuffer = (char*)NULL;
u32 PICSize = 0;
u32 PICNum = 0;

char *RegionStr[] = {
	"JAP",
	"USA",
	"EUR",
	"KOR",
	"ASN",
	"LTN",
	"UNK",
	"ALL",
};

unsigned char VISetFB[] =
{
    0x7C, 0xE3, 0x3B, 0x78,		//	mr      %r3, %r7
	0x38, 0x87, 0x00, 0x34,
	0x38, 0xA7, 0x00, 0x38,
	0x38, 0xC7, 0x00, 0x4C, 
};

void LoadAndRebuildChannelCache()
{
	channelCache = NULL;
	u32 size, i;
	UIDSYS *uid = (UIDSYS *)NANDLoadFile( "/sys/uid.sys", &size );

	if (uid == NULL)
		return;

	u32 numChannels = 0;

	for( i = 0; i * 12 < size; i++ )
	{
		u32 majorTitleID = uid[i].TitleID >> 32;

		switch (majorTitleID)
		{
			case 0x00000001: //IOSes
			case 0x00010000: //left over disc stuff
			case 0x00010005: //DLC
			case 0x00010008: //Hidden
				break;
			default:
			{
				s32 fd = ESP_OpenContent(uid[i].TitleID,0);
				if (fd >= 0 )
				{
					numChannels++;
					IOS_Close(fd);
				}
			} break;
		}
	}

	channelCache = (ChannelCache*)NANDLoadFile("/sneek/cnlcache.bin", &i );
	if( channelCache == NULL )
	{
		channelCache = (ChannelCache*)malloca(sizeof(ChannelCache),32);
		channelCache->numChannels = 0;
	}

	if (numChannels != channelCache->numChannels || i != sizeof(ChannelCache) + sizeof(ChannelInfo) * numChannels){ // rebuild
		free(channelCache);
		channelCache = (ChannelCache*)malloca(sizeof(ChannelCache) + sizeof(ChannelInfo) * numChannels,32);
		channelCache->numChannels = 0;
		for (i = 0; i * 12 < size; i++){
			u32 majorTitleID = uid[i].TitleID >> 32;
			switch (majorTitleID){
				case 0x00000001: //IOSes
				case 0x00010000: //left over disc stuff
				case 0x00010005: //DLC
				case 0x00010008: //Hidden
					break;
				default:
				{
					s32 fd = ESP_OpenContent(uid[i].TitleID,0);
					if (fd >= 0){
						IOS_Seek(fd,0xF0,SEEK_SET);
						u32 j;
						for (j = 0; j < 40; j++){
							IOS_Seek(fd,1,SEEK_CUR);
							IOS_Read(fd,&channelCache->channels[channelCache->numChannels].name[j],1);
						}
						channelCache->channels[channelCache->numChannels].name[40] = 0;
						channelCache->channels[channelCache->numChannels].titleID = uid[i].TitleID;
						channelCache->numChannels += 1;
						IOS_Close(fd);
					}
				} break;
			}
		}
		NANDWriteFileSafe( "/sneek/cnlcache.bin", channelCache,sizeof(ChannelCache) + sizeof(ChannelInfo) * channelCache->numChannels);
	}
	free(uid);
}

u32 SMenuFindOffsets( void *ptr, u32 SearchSize )
{
	u32 i;
	u32 r13  = 0;

	FBOffset = 0;
	FBEnable = 0;
	WPad	 = (u32*)NULL;
		
	//dbgprintf("ES:Start:%p Len:%08X\n", ptr, SearchSize );

	while(1)
	{
		for( i = 0; i < SearchSize; i+=4 )
		{
			if( *(u32*)(ptr+i) >> 16 == 0x3DA0 && r13 == 0 )
			{
				r13 = ((*(u32*)(ptr+i)) & 0xFFFF) << 16;
				r13|= (*(u32*)(ptr+i+4)) & 0xFFFF;
			}

			if( memcmp( ptr+i, VISetFB, sizeof(VISetFB) ) == 0 && FBEnable == 0 )
			{
				FBEnable = ( *(u32*)(ptr+i+sizeof(VISetFB)) );

				if( FBEnable & 0x8000 )
				{
					FBEnable = ((~FBEnable) & 0xFFFF) + 1;
					FBEnable = (r13 - FBEnable) & 0x7FFFFFF;
				} else {
					FBEnable = FBEnable & 0xFFFF;
					FBEnable = (r13 + FBEnable) & 0x7FFFFFF;
				}
				FBOffset = FBEnable + 0x18;
			}


			//Wpad pattern new
			if( (*(u32*)(ptr+i+0x00)) >> 16 == 0x1C03		&&		//  mulli   %r0, %r3, 0x688
				(*(u32*)(ptr+i+0x04)) >> 16 == 0x3C60		&&		//  lis     %r3, inside_kpads@h
				(*(u32*)(ptr+i+0x08)) >> 16 == 0x3863		&&		//  addi    %r3, %r3, inside_kpads@l
				(*(u32*)(ptr+i+0x0C))	    == 0x7C630214	&&		//  add     %r3, %r3, %r0
				(*(u32*)(ptr+i+0x10)) >> 16 == 0xD023		&&		//  stfs    %fp1, 0xF0(%r3)
				(*(u32*)(ptr+i+0x18))	    == 0x4E800020			//  blr
				)
			{
				if( *(u32*)(ptr+i+0x08) & 0x8000 )
					WPad = (u32*)( ((((*(u32*)(ptr+i+0x04)) & 0xFFFF) << 16) - (((~(*(u32*)(ptr+i+0x08))) & 0xFFFF)+1) ) & 0x7FFFFFF );
				else
					WPad = (u32*)( ((((*(u32*)(ptr+i+0x04)) & 0xFFFF) << 16) + ((*(u32*)(ptr+i+0x08)) & 0xFFFF)) & 0x7FFFFFF );
			}

			//WPad pattern old
			if( (*(u32*)(ptr+i+0x00)) >> 16 == 0x3C80		&&		//  lis     %r3, inside_kpads@h
				(*(u32*)(ptr+i+0x04))		== 0x5460502A	&&		//  slwi    %r0, %r3, 10
				(*(u32*)(ptr+i+0x08)) >> 16 == 0x3884		&&		//  addi    %r3, %r3, inside_kpads@l
				(*(u32*)(ptr+i+0x0C))	    == 0x7C640214	&&		//  add     %r3, %r4, %r0
				(*(u32*)(ptr+i+0x10)) >> 16 == 0xD023		&&		//  stfs    %fp1, 0xF0(%r3)
				(*(u32*)(ptr+i+0x18))	    == 0x4E800020			//  blr
				)
			{
				if( *(u32*)(ptr+i+0x08) & 0x8000 )
					WPad = (u32*)( ((((*(u32*)(ptr+i+0x00)) & 0xFFFF) << 16) - (((~(*(u32*)(ptr+i+0x08))) & 0xFFFF)+1) ) & 0x7FFFFFF );
				else
					WPad = (u32*)( ((((*(u32*)(ptr+i+0x00)) & 0xFFFF) << 16) + ((*(u32*)(ptr+i+0x08)) & 0xFFFF)) & 0x7FFFFFF );
			}
			
			if( r13 && FBEnable && FBOffset && WPad != NULL )
			{
				switch( *(vu32*)(FBEnable+0x20) )
				{
					case VI_NTSC:
						FBSize = 304*480*4;
						break;
					case VI_EUR60:
						FBSize = 320*480*4;
						break;
					default:
						dbgprintf("ES:SMenuFindOffsets():Invalid Video mode:%d\n", *(vu32*)(FBEnable+0x20) );
						break;
				}

				return 1;
			}
		}
	}
	return 0;
}
void SMenuInit( u64 TitleID, u16 TitleVersion )
{
	int i;

	value	= 0;
	Freeze	= 0;
	ShowMenu= 0;
	MenuType= 0;
	SLock	= 0;
	PosX	= 0;
	ScrollX	= 0;
	PosValX	= 0;
	Hits	= 0;
	edit	= 0;
	PICBuffer = (char*)NULL;

	Offsets		= (u32*)malloca( sizeof(u32) * MAX_HITS, 32 );
	FB			= (u32*)malloca( sizeof(u32) * MAX_FB, 32 );

	for( i=0; i < MAX_FB; ++i )
		FB[i] = 0;
//Patches and SNEEK Menu
	switch( TitleID )
	{
		case 0x0000000100000002LL:
		{
			switch( TitleVersion )
			{
				case 482:	// EUR 4.2
				{
					//Wii-Disc Region free hack
					*(u32*)0x0137DC90 = 0x4800001C;
					*(u32*)0x0137E4E4 = 0x60000000;

					//Disc autoboot
					//*(u32*)0x0137AD5C = 0x48000020;
					//*(u32*)0x013799A8 = 0x60000000;

					//BS2Report
					//*(u32*)0x137AEC4 = 0x481B22BC;
					
				} break;
				case 514:	// EUR 4.3
				{
					//Wii-Disc Region free hack
					*(u32*)0x0137DE28 = 0x4800001C;
					*(u32*)0x0137E7A4 = 0x38000001;

					//GC-Disc Region free hack
					*(u32*)0x137DAEC = 0x7F60DB78;
									
					//Autoboot disc
					//*(u32*)0x0137AEF4 = 0x48000020;
					//*(u32*)0x01379B40 = 0x60000000;
					
				} break;
				case 481:	// USA 4.2
				{
					//Wii-Disc Region free hack
					*(u32*)0x0137DBE8 = 0x4800001C;
					*(u32*)0x0137E43C = 0x60000000;

				} break;
				case 513:	// USA 4.3
				{
					//Wii-Disc Region free hack
					*(u32*)0x0137DD80 = 0x4800001C;
					*(u32*)0x0137E5D4 = 0x60000000;

					//GC-Disc Region free hack
					*(u32*)0x137DA44 = 0x7F60DB78;
				} break;
			}
		} break;
	}
}
void SMenuAddFramebuffer( void )
{
	u32 i,j,f;

	if( *(vu32*)FBEnable != 1 )
		return;

	FrameBuffer = (*(vu32*)FBOffset) & 0x7FFFFFFF;

	for( i=0; i < MAX_FB; i++)
	{
		if( FB[i] )	//add a new entry
			continue;
		
		//check if we already know this address
		f=0;
		for( j=0; j<i; ++j )
		{
			if( FrameBuffer == FB[j] )	// already known!
			{
				f=1;
				return;
			}
		}
		if( !f && FrameBuffer && FrameBuffer < 0x14000000 )	// add new entry
		{
			dbgprintf("ES:Added new FB[%d]:%08X\n", i, FrameBuffer );
			FB[i] = FrameBuffer;
		}
	}
}
void SMenuDraw( void )
{
	u32 i,j;
	s32 EntryCount=0;

	if( *(vu32*)FBEnable != 1 )
		return;

	if( ShowMenu == 0 )
		return;

	if( DICfg == NULL )
		return;

	if( DICfg->Config & CONFIG_SHOW_COVERS )
		EntryCount = 8;
	else
		EntryCount = 20;

	for( i=0; i < MAX_FB; i++)
	{
		if( FB[i] == 0 )
			continue;

		if( DICfg->Region > ALL )
			DICfg->Region = ALL;

		if( MenuType != 3 )
		{
			if( FSUSB )
			{
				PrintFormat( FB[i], MENU_POS_X, 20, "UNEEK+DI %s  Games:%d  Region:%s", __DATE__, DICfg->Gamecount, RegionStr[DICfg->Region] );
			} else {
				PrintFormat( FB[i], MENU_POS_X, 20, "SNEEK+DI %s  Games:%d  Region:%s", __DATE__, DICfg->Gamecount, RegionStr[DICfg->Region] );
			}
		}

		switch( MenuType )
		{
			case 0:
			{
				u32 gRegion = 0;

				switch( *(u8*)(DICfg->GameInfo[PosX+ScrollX] + 3) )
				{
					case 'X':	// hamster heroes uses this for some reason
					case 'A':
					case 'Z':
						gRegion =  ALL;
						break;
					case 'E':
						gRegion =  USA;
						break;
					case 'F':	// France
					case 'I':	// Italy
					case 'U':	// United Kingdom
					case 'S':	// Spain
					case 'D':	// Germany
					case 'P':	
						gRegion =  EUR;
						break;
					case 'J':
						gRegion =  JAP;
						break;
					default:
						gRegion =  UNK;
						break;
				}

				PrintFormat( FB[i], MENU_POS_X, 20+16, "Press PLUS for settings  Press MINUS for dumping" );
				
				PrintFormat( FB[i], MENU_POS_X, MENU_POS_Y, "GameRegion:%s", RegionStr[gRegion] );

				PrintFormat( FB[i], MENU_POS_X + 420, MENU_POS_Y,"Press HOME for Channels");

				for( j=0; j<EntryCount; ++j )
				{
					if( j+ScrollX >= DICfg->Gamecount )
						break;

					if( *(vu32*)(DICfg->GameInfo[ScrollX+j]+0x1C) == 0xc2339f3d )
						PrintFormat( FB[i], MENU_POS_X, MENU_POS_Y+32+16*j, "%.40s (GC)", DICfg->GameInfo[ScrollX+j] + 0x20 );
					else if( *(vu32*)(DICfg->GameInfo[ScrollX+j]+0x18) == 0x5D1C9EA3 )
						PrintFormat( FB[i], MENU_POS_X, MENU_POS_Y+32+16*j, "%.40s (Wii)", DICfg->GameInfo[ScrollX+j] + 0x20 );
					else
						PrintFormat( FB[i], MENU_POS_X, MENU_POS_Y+32+16*j, "%.40s (Invalid)", DICfg->GameInfo[ScrollX+j] + 0x20 );

					if( j == PosX )
						PrintFormat( FB[i], 0, MENU_POS_Y+32+16*j, "-->");
				}

				if( DICfg->Config & CONFIG_SHOW_COVERS )
				{
					if (curDVDCover)
					{
						DrawImage( FB[i], MENU_POS_X, MENU_POS_Y+(j+2)*16, curDVDCover );
						//PrintFormat(FB[i],MENU_POS_X,MENU_POS_Y+12*16,(char*) curDVDCover);
					} else
						PrintFormat( FB[i], MENU_POS_X+6*12, MENU_POS_Y+(j+6)*16, "no cover image found!" );
				}

				PrintFormat( FB[i], MENU_POS_X+575, MENU_POS_Y+16*22, "%d/%d", ScrollX/EntryCount + 1, DICfg->Gamecount/EntryCount + (DICfg->Gamecount % EntryCount > 0));
				
				sync_after_write( (u32*)(FB[i]), FBSize );
			} break;

			case 4:
			{
				PrintFormat( FB[i], MENU_POS_X, 20+16, "Close the HOME menu before launching!!!" );
				PrintFormat( FB[i], MENU_POS_X, MENU_POS_Y, "Installed Channels:%u", channelCache->numChannels);

				for( j=0; j<EntryCount; ++j )
				{
					if( j+ScrollX >= channelCache->numChannels )
						break;

					PrintFormat( FB[i], MENU_POS_X, MENU_POS_Y+32+16*j, "%.40s", channelCache->channels[ScrollX+j].name);

					if( j == PosX )
						PrintFormat( FB[i], 0, MENU_POS_Y+32+16*j, "-->");
				}

				if( DICfg->Config & CONFIG_SHOW_COVERS )
				{
					if (curDVDCover){
						DrawImage(FB[i],MENU_POS_X,MENU_POS_Y+(j+2)*16,curDVDCover);
					}
					else
						PrintFormat(FB[i],MENU_POS_X,MENU_POS_Y+(j+2)*16,"no cover image found!");
				}

				PrintFormat( FB[i], MENU_POS_X+575, MENU_POS_Y+16*21, "%d/%d", ScrollX/EntryCount + 1, channelCache->numChannels/EntryCount + (channelCache->numChannels % EntryCount > 0));

				sync_after_write( (u32*)(FB[i]), FBSize );
			} break;

			case 1:
			{
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*0, "Game Region            :%s", RegionStr[DICfg->Region] );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*1, "__fwrite patch         :%s", (DICfg->Config&CONFIG_PATCH_FWRITE) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*2, "MotionPlus video       :%s", (DICfg->Config&CONFIG_PATCH_MPVIDEO) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*3, "Videomode patch        :%s", (DICfg->Config&CONFIG_PATCH_VIDEO) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*4, "Skip read errors (dump):%s", (DICfg->Config&CONFIG_DUMP_ERROR_SKIP) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*5, "Disc dump mode         :%s", (DICfg->Config&CONFIG_DUMP_MODE) ? "Ex" : "RAW" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*6, "Display covers         :%s", (DICfg->Config&CONFIG_SHOW_COVERS) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*7, "Autoupdate game list   :%s", (DICfg->Config&CONFIG_AUTO_UPDATE_LIST) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*8, "Game debugging         :%s", (DICfg->Config&CONFIG_DEBUG_GAME) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*9, "Debugger wait          :%s", (DICfg->Config&CONFIG_DEBUG_GAME_WAIT) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*11,"Fake console region    :%s", (DICfg->Config&CONFIG_FAKE_CONSOLE_RG) ? "On" : "Off" );
				
				switch( (DICfg->Config&HOOK_TYPE_MASK) )
				{
					case HOOK_TYPE_VSYNC:
						PrintFormat( FB[i], MENU_POS_X+50, 104+16*10, "Hook type              :%s", "VIWaitForRetrace" );
					break;
					case HOOK_TYPE_OSLEEP:
						PrintFormat( FB[i], MENU_POS_X+50, 104+16*10, "Hook type              :%s", "OSSleepThread" );
					break;
					//case HOOK_TYPE_AXNEXT:
					//	PrintFormat( FB[i], MENU_POS_X+60, 104+16*9, "Hook type       :%s", "__AXNextFrame" );
					//break;
					default:
						DICfg->Config &= ~HOOK_TYPE_MASK;
						DICfg->Config |= HOOK_TYPE_OSLEEP;
					break;
				}

				PrintFormat( FB[i], MENU_POS_X+50, 104+16*13, "save config" );
				PrintFormat( FB[i], MENU_POS_X+50, 104+16*14, "recreate game cache(restarts!!)" );
				if( FSUSB )
					PrintFormat( FB[i], MENU_POS_X+50, 104+16*15, "Boot NMM" );				

				PrintFormat( FB[i], MENU_POS_X+30, 40+64+16*PosX, "-->");
				sync_after_write( (u32*)(FB[i]), FBSize );
			} break;

			case 2:
			{
				MenuType = DumpDoTick(i);

			} break;
			case 3:
			{
				memcpy( (void*)(FB[i]), PICBuffer, FBSize );
			} break;
			default:
			{
				MenuType = 0;
				ShowMenu = 0;
			} break;
		}
	}
}
void LoadDVDCover()
{
	if (curDVDCover != NULL)
		free(curDVDCover);

	curDVDCover = NULL;
	
	if (DICfg == NULL || PosX + ScrollX >= DICfg->Gamecount )
		return;

	char* imgPathBuffer = (char*)malloca(160,32);
	_sprintf( imgPathBuffer, "/sneek/covers/%s.raw", DICfg->GameInfo[PosX+ScrollX]);

	curDVDCover = LoadImage(imgPathBuffer);

	if (curDVDCover == NULL)
	{
		_sprintf( imgPathBuffer, "/sneek/covers/%s.bmp", DICfg->GameInfo[PosX+ScrollX]);
		curDVDCover = LoadImage(imgPathBuffer);
	}

	free(imgPathBuffer);
}
void LoadChannelCover()
{
	if (curDVDCover != NULL)
		free(curDVDCover);

	curDVDCover = NULL;
	
	if (channelCache == NULL || PosX + ScrollX >= channelCache->numChannels )
		return;

	char* imgPathBuffer = (char*)malloca(160,32);
	u32 minorTitleID = channelCache->channels[PosX+ScrollX].titleID & 0xFFFFFFFF;

	_sprintf( imgPathBuffer, "/sneek/covers/%c%c%c%c.raw", minorTitleID >> 24 & 0xFF, minorTitleID >> 16 & 0xFF, minorTitleID >> 8  & 0xFF, minorTitleID & 0xFF );
	curDVDCover = LoadImage(imgPathBuffer);

	if (curDVDCover == NULL)
	{
		_sprintf( imgPathBuffer, "/sneek/covers/%c%c%c%c.bmp", minorTitleID >> 24 & 0xFF, minorTitleID >> 16 & 0xFF, minorTitleID >> 8  & 0xFF, minorTitleID & 0xFF );
		curDVDCover = LoadImage(imgPathBuffer);
	}

	free(imgPathBuffer);
}
void SMenuReadPad ( void )
{
	s32 EntryCount=0;

	memcpy( &GCPad, (u32*)0xD806404, sizeof(u32) * 2 );

	if( ( GCPad.Buttons & 0x1F3F0000 ) == 0 && ( *WPad & 0x0000FFFF ) == 0 )
	{
		SLock = 0;
		return;
	}

	if( SLock != 0 )
		return;

	if( GCPad.Start || (*WPad&WPAD_BUTTON_1) )
	{
		ShowMenu = !ShowMenu;
		if(ShowMenu)
		{
			if( MenuType == 0 && (DICfg->Config & CONFIG_SHOW_COVERS) )
				LoadDVDCover();
		}
		SLock = 1;
	}

	if( !ShowMenu )
		return;
		
	if( DICfg->Config & CONFIG_SHOW_COVERS )
		EntryCount = 8;
	else
		EntryCount = 20;

	if( (GCPad.B || (*WPad&WPAD_BUTTON_B) ) && SLock == 0 )
	{
		if( MenuType == 3 )
			free( PICBuffer );
		
		if( curDVDCover != NULL )
			free(curDVDCover);

		curDVDCover = NULL;

		if( MenuType == 0 )
		{
			ShowMenu = 0;
		}

		MenuType = 0;

		PosX	= 0;
		ScrollX	= 0;
		SLock	= 1;

		if( ShowMenu != 0 && DICfg->Config & CONFIG_SHOW_COVERS )
			LoadDVDCover();
	}

	if( (GCPad.X || (*WPad&WPAD_BUTTON_PLUS) ) && SLock == 0 && MenuType != 1 )
	{
		if( curDVDCover != NULL )
			free(curDVDCover);

		curDVDCover = NULL;

		MenuType = 1;

		PosX	= 0;
		ScrollX	= 0;
		SLock	= 1;
	}

	if( (GCPad.Y || (*WPad&WPAD_BUTTON_MINUS) ) && SLock == 0 && MenuType != 2 )
	{
		if( curDVDCover != NULL )
			free(curDVDCover);

		curDVDCover = NULL;

		MenuType = 2;

		PosX	= 0;
		ScrollX	= 0;
		SLock	= 1;
	}

	if( (GCPad.Z || (*WPad&WPAD_BUTTON_2) ) && SLock == 0 && MenuType != 3 )
	{
		if( curDVDCover != NULL )
			free(curDVDCover);

		curDVDCover = NULL;

		MenuType= 3;
		PICSize	= 0;
		PICNum	= 0;

		s32 fd = IOS_Open("/scrn_00.raw", 1 );
		if( fd >= 0 )
		{
			PICSize = IOS_Seek( fd, 0, SEEK_END );
			IOS_Seek( fd, 0, 0 );
			PICBuffer = (char*)malloca( FBSize, 32 );
			IOS_Read( fd, PICBuffer, FBSize );
			IOS_Close( fd );
		}

		PosX	= 0;
		ScrollX	= 0;
		SLock	= 1;
	}

	if( *WPad & WPAD_BUTTON_HOME && MenuType != 4 )
	{
		if( curDVDCover != NULL )
			free(curDVDCover);

		curDVDCover = NULL;

		MenuType= 4;
		PosX	= 0;
		ScrollX	= 0;
		SLock	= 1;

		if( DICfg->Config & CONFIG_SHOW_COVERS )
			LoadChannelCover();
	}

	switch( MenuType )
	{
		case 4: //channel list
		{
			if( GCPad.A || (*WPad&WPAD_BUTTON_A) )
			{
				if (curDVDCover != NULL)
					free(curDVDCover);
				curDVDCover = NULL;
				ShowMenu = 0;
				LaunchTitle(channelCache->channels[PosX + ScrollX].titleID);
				SLock = 1;
				break;
			}
			if( GCPad.Up || (*WPad&WPAD_BUTTON_UP) )
			{
				if( PosX ){
					PosX--;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadChannelCover();
				}
				else if( ScrollX )
				{
					ScrollX--;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadChannelCover();
				}

				SLock = 1;
			} else if( GCPad.Down || (*WPad&WPAD_BUTTON_DOWN) )
			{
				if( PosX >= EntryCount-1 )
				{
					if( PosX+ScrollX+1 < channelCache->numChannels )
					{
						ScrollX++;
						if( DICfg->Config & CONFIG_SHOW_COVERS )
							LoadChannelCover();
					}
				} else if ( PosX+ScrollX+1 < channelCache->numChannels ){
					PosX++;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadChannelCover();
				}

				SLock = 1;
			} else if( GCPad.Right || (*WPad&WPAD_BUTTON_RIGHT) )
			{
				if( ScrollX/EntryCount*EntryCount + EntryCount < channelCache->numChannels )
				{
					PosX	= 0;
					ScrollX = ScrollX/EntryCount*EntryCount + EntryCount;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadChannelCover();
				} else {
					PosX	= 0;
					ScrollX	= 0;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadChannelCover();
				}

				SLock = 1; 
			} else if( GCPad.Left || (*WPad&WPAD_BUTTON_LEFT) )
			{
				if( ScrollX/EntryCount*EntryCount - EntryCount > 0 )
				{
					PosX	= 0;
					ScrollX-= EntryCount;
				} else {
					PosX	= 0;
					ScrollX	= 0;
				}
				if( DICfg->Config & CONFIG_SHOW_COVERS )
					LoadChannelCover();

				SLock = 1; 
			}
		} break;
		case 0:			// Game list
		{
			if( GCPad.A || (*WPad&WPAD_BUTTON_A) )
			{
				DVDSelectGame( PosX+ScrollX );
				if (curDVDCover != NULL)
					free(curDVDCover);
				curDVDCover = NULL;
				ShowMenu = 0;
				SLock = 1;
			}
			if( GCPad.Up || (*WPad&WPAD_BUTTON_UP) )
			{
				if( PosX )
				{
					PosX--;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadDVDCover();
				}
				else if( ScrollX )
				{
					ScrollX--;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadDVDCover();
				}

				SLock = 1;
			} else if( GCPad.Down || (*WPad&WPAD_BUTTON_DOWN) )
			{
				if( PosX >= EntryCount-1 )
				{
					if( PosX+ScrollX+1 < DICfg->Gamecount )
					{
						ScrollX++;
						if( DICfg->Config & CONFIG_SHOW_COVERS )
							LoadDVDCover();
					}
				} else if ( PosX+ScrollX+1 < DICfg->Gamecount )
				{
					PosX++;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadDVDCover();
				}

				SLock = 1;
			} else if( GCPad.Right || (*WPad&WPAD_BUTTON_RIGHT) )
			{
				if( ScrollX/EntryCount*EntryCount + EntryCount < DICfg->Gamecount )
				{
					PosX	= 0;
					ScrollX = ScrollX/EntryCount*EntryCount + EntryCount;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadDVDCover();
				} else {
					PosX	= 0;
					ScrollX	= 0;
					if( DICfg->Config & CONFIG_SHOW_COVERS )
						LoadDVDCover();
				}

				SLock = 1; 
			} else if( GCPad.Left || (*WPad&WPAD_BUTTON_LEFT) )
			{
				if( ScrollX/EntryCount*EntryCount - EntryCount > 0 )
				{
					PosX	= 0;
					ScrollX-= EntryCount;
				} else {
					PosX	= 0;
					ScrollX	= 0;
				}

				if( DICfg->Config & CONFIG_SHOW_COVERS )
					LoadDVDCover();

				SLock = 1; 
			}
		} break;
		case 1:		//SNEEK Settings
		{
			if( GCPad.A || (*WPad&WPAD_BUTTON_A) )
			{
				switch(PosX)
				{
					case 0:
					{
						if( DICfg->Region == LTN )
							DICfg->Region = JAP;
						else
							DICfg->Region++;
						SLock = 1;
						DVDReinsertDisc=true;
					} break;
					case 1:
					{
						DICfg->Config ^= CONFIG_PATCH_FWRITE;
						DVDReinsertDisc=true;
					} break;
					case 2:
					{
						DICfg->Config ^= CONFIG_PATCH_MPVIDEO;
						DVDReinsertDisc=true;
					} break;
					case 3:
					{
						DICfg->Config ^= CONFIG_PATCH_VIDEO;
						DVDReinsertDisc=true;
					} break;
					case 4:
					{
						DICfg->Config ^= CONFIG_DUMP_ERROR_SKIP;
					} break;
					case 5:
					{
						DICfg->Config ^= CONFIG_DUMP_MODE;
					} break;
					case 6:
					{
						DICfg->Config ^= CONFIG_SHOW_COVERS;
					} break;
					case 7:
					{
						DICfg->Config ^= CONFIG_AUTO_UPDATE_LIST;
					} break;
					case 8:
					{
						DICfg->Config ^= CONFIG_DEBUG_GAME;
						DVDReinsertDisc=true;
					} break;
					case 9:
					{
						DICfg->Config ^= CONFIG_DEBUG_GAME_WAIT;
						DVDReinsertDisc=true;
					} break;
					case 10:
					{
						if( (DICfg->Config & HOOK_TYPE_MASK) == HOOK_TYPE_OSLEEP )
						{
							DICfg->Config &= ~HOOK_TYPE_MASK;
							DICfg->Config |= HOOK_TYPE_VSYNC;
						} else {
							DICfg->Config += HOOK_TYPE_VSYNC;								
						}

						DVDReinsertDisc=true;
					} break;
					case 11:
					{
						DICfg->Config ^= CONFIG_FAKE_CONSOLE_RG;
					} break;
					case 13:
					{
						if( DVDReinsertDisc )
							DVDSelectGame( DICfg->SlotID );

						DVDWriteDIConfig( DICfg );

						DVDReinsertDisc=false;
					} break;
					case 14:
					{
						DICfg->Gamecount = 0;
						DICfg->Config	|= CONFIG_AUTO_UPDATE_LIST;
						DVDWriteDIConfig( DICfg );
						LaunchTitle( 0x0000000100000002LL );	
					} break;
					case 15:
					{
						LaunchTitle( 0x0000000100000100LL );						
					} break;
				}
				SLock = 1;
			}
			if( GCPad.Up || (*WPad&WPAD_BUTTON_UP) )
			{
				if( PosX )
					PosX--;
				else {
					if( FSUSB )
						PosX  = 15;
					else
						PosX = 14;
				}

				if( PosX == 12 )
					PosX  = 11;

				SLock = 1;
			} else if( GCPad.Down || (*WPad&WPAD_BUTTON_DOWN) )
			{
				if( FSUSB )
				{
					if( PosX >= 15 )
					{
						PosX=0;
					} else 
						PosX++;
				} else {
					if( PosX >= 14 )
					{
						PosX=0;
					} else 
						PosX++;
				}

				if( PosX == 12 )
					PosX  = 13;

				SLock = 1;
			}

			if( GCPad.Right || (*WPad&WPAD_BUTTON_RIGHT) )
			{
				switch( PosX )
				{
					case 0:
					{
						if( DICfg->Region == LTN )
							DICfg->Region = JAP;
						else
							DICfg->Region++;
						DVDReinsertDisc=true;
					} break;
					case 1:
					{
						DICfg->Config ^= CONFIG_PATCH_FWRITE;
						DVDReinsertDisc=true;
					} break;
					case 2:
					{
						DICfg->Config ^= CONFIG_PATCH_MPVIDEO;
						DVDReinsertDisc=true;
					} break;
					case 3:
					{
						DICfg->Config ^= CONFIG_PATCH_VIDEO;
						DVDReinsertDisc=true;
					} break;
					case 4:
					{
						DICfg->Config ^= CONFIG_DUMP_ERROR_SKIP;
					} break;
					case 5:
					{
						DICfg->Config ^= CONFIG_DUMP_MODE;
					} break;
					case 6:
					{
						DICfg->Config ^= CONFIG_SHOW_COVERS;
					} break;
					case 7:
					{
						DICfg->Config ^= CONFIG_AUTO_UPDATE_LIST;
					} break;
					case 8:
					{
						DICfg->Config ^= CONFIG_DEBUG_GAME;
						DVDReinsertDisc=true;
					} break;
					case 9:
					{
						DICfg->Config ^= CONFIG_DEBUG_GAME_WAIT;
						DVDReinsertDisc=true;
					} break;
					case 10:
					{
						if( (DICfg->Config & HOOK_TYPE_MASK) == HOOK_TYPE_OSLEEP )
						{
							DICfg->Config &= ~HOOK_TYPE_MASK;
							DICfg->Config |= HOOK_TYPE_VSYNC;
						} else {
							DICfg->Config += HOOK_TYPE_VSYNC;								
						}
						
						DVDReinsertDisc=true;
					} break;
					case 11:
					{
						DICfg->Config ^= CONFIG_FAKE_CONSOLE_RG;
					} break;
				}
				SLock = 1;
			} else if( GCPad.Left || (*WPad&WPAD_BUTTON_LEFT) )
			{
				switch( PosX )
				{
					case 0:
					{
						if( DICfg->Region == JAP )
							DICfg->Region = LTN;
						else
							DICfg->Region--;
						DVDReinsertDisc=true;
					} break;
					case 1:
					{
						DICfg->Config ^= CONFIG_PATCH_FWRITE;
						DVDReinsertDisc=true;
					} break;
					case 2:
					{
						DICfg->Config ^= CONFIG_PATCH_MPVIDEO;
						DVDReinsertDisc=true;
					} break;
					case 3:
					{
						DICfg->Config ^= CONFIG_PATCH_VIDEO;
						DVDReinsertDisc=true;
					} break;
					case 4:
					{
						DICfg->Config ^= CONFIG_DUMP_ERROR_SKIP;
					} break;
					case 5:
					{
						DICfg->Config ^= CONFIG_DUMP_MODE;
					} break;
					case 6:
					{
						DICfg->Config ^= CONFIG_SHOW_COVERS;
					} break;
					case 7:
					{
						DICfg->Config ^= CONFIG_AUTO_UPDATE_LIST;
					} break;
					case 8:
					{
						DICfg->Config ^= CONFIG_DEBUG_GAME;
						DVDReinsertDisc=true;
					} break;
					case 9:
					{
						DICfg->Config ^= CONFIG_DEBUG_GAME_WAIT;
						DVDReinsertDisc=true;
					} break;
					case 10:
					{
						if( (DICfg->Config & HOOK_TYPE_MASK) == HOOK_TYPE_OSLEEP )
						{
							DICfg->Config &= ~HOOK_TYPE_MASK;
							DICfg->Config |= HOOK_TYPE_VSYNC;
						} else {
							DICfg->Config += HOOK_TYPE_VSYNC;								
						}
						
						DVDReinsertDisc=true;
					} break;
					case 11:
					{
						DICfg->Config ^= CONFIG_FAKE_CONSOLE_RG;
					} break;
				}
				SLock = 1;
			} 

		} break;
		case 2:
		{
			if( GCPad.A || (*WPad&WPAD_BUTTON_A) )
			{
				if( DVDStatus == 2 && DVDType > 0 )
				{
					if( (DICfg->Config & CONFIG_DUMP_MODE) && DVDType != 1 )	// Ex format only works with Wii games
						DVDStatus = 6;	// EX
					else
						DVDStatus = 3;	// RAW

					thread_set_priority( 0, 0x79 );
				}
				SLock = 1;
			}
		} break;
		case 3:
		{
			u32 Update = false;
			if( GCPad.Left || (*WPad&WPAD_BUTTON_LEFT) )
			{
				if( PICNum > 0 )
				{
					PICNum--;
					Update = true;
				}
				SLock = 1;
			}
			if( GCPad.Right || (*WPad&WPAD_BUTTON_RIGHT) )
			{
				PICNum++;
				Update = true;
					
				SLock = 1;
			}

			if( Update )
			{
				char *Path = (char*)malloca( 32, 32 );
				_sprintf( Path, "/scrn_%02X.raw", PICNum );

				s32 fd = IOS_Open( Path, 1 );
				if( fd >= 0 )
				{
					PICSize = IOS_Seek( fd, 0, SEEK_END );
					IOS_Seek( fd, 0, 0 );
					IOS_Read( fd, PICBuffer, PICSize );
					IOS_Close( fd );
				}

				free(Path);
			}
		} break;
	}
}
#ifdef CHEATMENU
void SCheatDraw( void )
{
	u32 i,j;
	offset = (u32*)0x007D0500;
	
	if( Freeze == 0xdeadbeef )
	{
		*offset = value;
	}

	if( *(vu32*)FBEnable != 1 )
		return;

	if( ShowMenu == 0 )
		return;

	for( i=0; i<3; i++)
	{
		if( FB[i] == 0 )
			continue;
		
		switch( ShowMenu )
		{
			case 0:
			{
				PrintFormat( FB[i], MENU_POS_X, 40, "SNEEK+DI " __DATE__ "  Cheater!!!");
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*0, "Search value..." );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*1, "RAM viewer...(NYI)" );

				PrintFormat( FB[i], MENU_POS_X+80-6*3, 104+16*PosX, "-->");

			} break;
			case 2:
			{
				PrintFormat( FB[i], MENU_POS_X, 40, "SNEEK+DI %s  Search value", __DATE__ );
				PrintFormat( FB[i], MENU_POS_X, 104+16*-1, "Hits :%08X", Hits );
				PrintFormat( FB[i], MENU_POS_X+6*6+PosValX*6, 108, "_" );
				PrintFormat( FB[i], MENU_POS_X, 104+16*0, "Value:%08X(%d:%u)", value, value, value );

				for( j=0; j < Hits; ++j )
				{
					if( j > 10 )
						break;
					PrintFormat( FB[i], MENU_POS_X, 104+32+16*j, "%08X:%08X(%d:%u)", Offsets[j], *(u32*)(Offsets[j]), *(u32*)(Offsets[j]), *(u32*)(Offsets[j]) );
				}
				
			} break;
			case 3:
			{
				PrintFormat( FB[i], MENU_POS_X, 32, "SNEEK+DI %s  edit value", __DATE__ );

				for( j=0; j < Hits; ++j )
				{
					if( j > 20 )
						break;
					if( j == PosX && edit )
						PrintFormat( FB[i], MENU_POS_X+9*6+PosValX*6, 64+18*j+2, "_" );
					PrintFormat( FB[i], MENU_POS_X, 64+18*j, "%08X:%08X(%d:%u)", Offsets[j], *(u32*)(Offsets[j]), *(u32*)(Offsets[j]), *(u32*)(Offsets[j]) );
				}

				PrintFormat( FB[i], MENU_POS_X-6*3, 64+18*PosX, "-->");
				
			} break;
			
		}
		//PrintFormat( FB[i], MENU_POS_X+80, 104+16*0, "%08X:%08X(%d)", offset, *offset, *offset );

		//if( Freeze == 0xdeadbeef )
		//	PrintFormat( FB[i], MENU_POS_X+80, 104+16*1, "Frozen!" );

		sync_after_write( (u32*)(FB[i]), FBSize );
	}
}
void SCheatReadPad ( void )
{
	int i;

	memcpy( &GCPad, (u32*)0xD806404, sizeof(u32) * 2 );

	if( ( GCPad.Buttons & 0x1F3F0000 ) == 0 && ( *WPad & 0x0000FFFF ) == 0 )
	{
		SLock = 0;
		return;
	}

	if( SLock == 0 )
	{
		if( (*WPad & ( WPAD_BUTTON_B | WPAD_BUTTON_1 )) == ( WPAD_BUTTON_B | WPAD_BUTTON_1 ) ||
			GCPad.Start )
		{
			ShowMenu = !ShowMenu;
			SLock = 1;
		}

		if( (*WPad & ( WPAD_BUTTON_1 | WPAD_BUTTON_2 )) == ( WPAD_BUTTON_1 | WPAD_BUTTON_2 ) ||
			GCPad.X	)
		{
			u8 *buf = (u8*)malloc( 40 );
			//memcpy( buf, (void*)(FB[0]), FBSize );

			dbgprintf("ES:Taking RamDump...");

			char *str = (char*)malloc( 32 );

			i=0;

			do
			{
				_sprintf( str, "/scrn_%02X.raw", i++ );
				s32 r = ISFS_CreateFile(str, 0, 3, 3, 3);
				if( r < 0  )
				{
					if( r != -105 )
					{
						dbgprintf("ES:ISFS_CreateFile():%d\n", r );
						free( buf );
						free( str );
						return;
					}
				} else {
					break;
				}
			} while(1);

			s32 fd = IOS_Open( str, 3 );
			if( fd < 0 )
			{
				dbgprintf("ES:IOS_Open():%d\n", fd );
				free( buf );
				free( str );
				return;
			}

			IOS_Write( fd, (void*)0, 24*1024*1024 );

			IOS_Close( fd );

			free( buf );
			free( str );

			dbgprintf("done\n");
			SLock = 1;
		}
	
		//if( !ShowMenu )
		//	return;

		switch( ShowMenu )
		{
			case 0:
			{
				if( (*WPad&WPAD_BUTTON_1) && SLock == 0 )
				{
					hexdump( (void*)0x5CBE60, 0x10 );
					SLock = 1;
				}
			} break;
			case 1:
			{
				if( GCPad.A || (*WPad&WPAD_BUTTON_A) )
				{
					ShowMenu= 2;
					SLock	= 1;
					PosValX	= 0;
				}
			} break;
			case 2:
			{
				if( GCPad.A || (*WPad&WPAD_BUTTON_A) )
				{
					Hits = 0;

					for( i=0; i < 0x01800000; i+=4 )
					{
						if( *(u32*)i == value )
						{
							if( Hits < MAX_HITS )
							{
								Offsets[Hits] = i;
							}
							Hits++;
						}
					}

					SLock	= 1;
				}
				if(  GCPad.B || (*WPad&WPAD_BUTTON_B) )
				{
					ShowMenu= 1;
					SLock	= 1;
				}
				if( GCPad.X || (*WPad&WPAD_BUTTON_PLUS) )
				{
					ShowMenu= 3;
					PosX	= 0;
					edit	= 0;
					SLock	= 1;
				}
				if(  GCPad.Left || (*WPad&WPAD_BUTTON_LEFT))
				{
					if( PosValX > 0 )
						PosValX--;
					SLock = 1;
				} else if(  GCPad.Right || (*WPad&WPAD_BUTTON_RIGHT))
				{
					if( PosValX < 7 )
						PosValX++;
					SLock = 1;
				}
				if( GCPad.Up || (*WPad&WPAD_BUTTON_UP))
				{
					if( ((value>>((7-PosValX)<<2)) & 0xF) == 0xF )
					{
						value &= ~(0xF << ((7-PosValX)<<2));
					} else {
						value += 0x1 << ((7-PosValX)<<2);
					}
					SLock = 1;
				} else if( GCPad.Down || (*WPad&WPAD_BUTTON_DOWN))
				{
					if( ((value>>((7-PosValX)<<2)) & 0xF) == 0x0 )
					{
						value |= (0xF << ((7-PosValX)<<2));
					} else {
						value -= 0x1 << ((7-PosValX)<<2);
					}
					SLock = 1;
				}
			} break;
			case 3:
			{
				if( *WPad == WPAD_BUTTON_A )
				{
					edit	= !edit;
					SLock	= 1;
					PosValX	= 0;
				}
				if( *WPad == WPAD_BUTTON_PLUS )
				{
					ShowMenu= 2;
					PosX	= 0;
					edit	= 0;
					SLock	= 1;
				}
				if( edit == 0 )
				{
					if( *WPad == WPAD_BUTTON_UP )
					{
						if( PosX > 0 )
							PosX--;

						SLock = 1;
					} else if( *WPad == WPAD_BUTTON_DOWN )
					{
						if( PosX <= Hits && PosX <= 20 )
							PosX++;
						else
							PosX = 0;

						SLock = 1;
					}
				} else {
					if( *WPad == WPAD_BUTTON_LEFT )
					{
						if( PosValX > 0 )
							PosValX--;
						SLock = 1;
					} else if( *WPad == WPAD_BUTTON_RIGHT )
					{
						if( PosValX < 7 )
							PosValX++;
						SLock = 1;
					}
					if( *WPad == WPAD_BUTTON_UP )
					{
						u32 val = *(vu32*)(Offsets[PosX]);

						if( ((val>>((7-PosValX)<<2)) & 0xF) == 0xF )
						{
							val &= ~(0xF << ((7-PosValX)<<2));
						} else {
							val += 0x1 << ((7-PosValX)<<2);
						}

						 *(vu32*)(Offsets[PosX]) = val;

						SLock = 1;
					} else if( *WPad == WPAD_BUTTON_DOWN )
					{
						u32 val = *(vu32*)(Offsets[PosX]);

						if( ((val>>((7-PosValX)<<2)) & 0xF) == 0x0 )
						{
							val |= (0xF << ((7-PosValX)<<2));
						} else {
							val -= 0x1 << ((7-PosValX)<<2);
						}

						 *(vu32*)(Offsets[PosX]) = val;

						SLock = 1;
					}
				}
			} break;
		}
	}
}
#endif
