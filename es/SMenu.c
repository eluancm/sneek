#include "SMenu.h"


u32 FrameBuffer	= 0;
u32 FBOffset	= 0;
u32 FBEnable	= 0;
u32	FBSize		= 0;
u32 *WPad		= NULL;
u32 *GameCount;

u32 ShowMenu=0;
u32 MenuType=0;
u32 DVDStatus = 0;
u32 DVDType = 0;
u32 DVDError=0;
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

u32 DVDErrorSkip=0;
u32 DVDErrorRetry=0;
u32 DVDTimer = 0;
u32 DVDTimeStart = 0;
u32 DVDSectorSize = 0;
u32 DVDOffset = 0;
u32 DVDOldOffset = 0;
u32 DVDSpeed = 0;
u32 DVDTimeLeft = 0;
s32 DVDHandle = 0;
char *DiscName	= (char*)NULL;
char *DVDTitle	= (char*)NULL;
char *DVDBuffer	= (char*)NULL;
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
						dbgprintf("ES:SMenuFindOffsets():Invalid Vide mode:%d\n", *(vu32*)(FBEnable+0x20) );
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
	DVDStatus = 0;
	DVDError=0;
	DICfg	= NULL;
	PICBuffer = (char*)NULL;

	Offsets		= (u32*)malloca( sizeof(u32) * MAX_HITS, 32 );
	GameCount	= (u32*)malloca( sizeof(u32), 32 );
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
					//Disc Region free hack
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
					//Disc Region free hack
					*(u32*)0x0137DE28 = 0x4800001C;
					*(u32*)0x0137E7A4 = 0x38000001;

					//Disable bannerbomb fix
					*(u32*)0x01380FC4 = 0x4E800020;
					
				} break;
				case 481:	// USA 4.2
				{
					//Disc Region free hack
					*(u32*)0x0137DBE8 = 0x4800001C;
					*(u32*)0x0137E43C = 0x60000000;

				} break;
				case 513:	// USA 4.3
				{
					//region free discs
					*(u32*)0x0137DD80 = 0x4800001C;
					*(u32*)0x0137E5D4 = 0x60000000;
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

	if( *(vu32*)FBEnable != 1 )
		return;

	if( ShowMenu == 0 )
		return;

	if( DICfg == NULL )
		return;

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
				PrintFormat( FB[i], MENU_POS_X, 20, "UNEEK+DI %s  Games:%d  Region:%s", __DATE__, *GameCount, RegionStr[DICfg->Region] );
			} else {
				PrintFormat( FB[i], MENU_POS_X, 20, "SNEEK+DI %s  Games:%d  Region:%s", __DATE__, *GameCount, RegionStr[DICfg->Region] );
			}
		}

		switch( MenuType )
		{
			case 0:
			{
				u32 gRegion = 0;

				switch( *(u8*)(DICfg->GameInfo[PosX+ScrollX] + 3) )
				{
					case 'A':
					case 'Z':
						gRegion =  ALL;
						break;
					case 'E':
						gRegion =  USA;
						break;
					case 'X':	// hamster heroes uses this for some reason
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

				for( j=0; j<10; ++j )
				{
					if( j+ScrollX >= *GameCount )
						break;

					if( *(vu32*)(DICfg->GameInfo[ScrollX+j]+0x1C) == 0xc2339f3d )
						PrintFormat( FB[i], MENU_POS_X, MENU_POS_Y+16+16*j, "%.40s (GC)", DICfg->GameInfo[ScrollX+j] + 0x20 );
					else if( *(vu32*)(DICfg->GameInfo[ScrollX+j]+0x18) == 0x5D1C9EA3 )
						PrintFormat( FB[i], MENU_POS_X, MENU_POS_Y+16+16*j, "%.40s (Wii)", DICfg->GameInfo[ScrollX+j] + 0x20 );
					else
						PrintFormat( FB[i], MENU_POS_X, MENU_POS_Y+16+16*j, "%.40s (Invalid)", DICfg->GameInfo[ScrollX+j] + 0x20 );

					if( j == PosX )
						PrintFormat( FB[i], 0, MENU_POS_Y+16+16*j, "-->");
				}

				if (curDVDCover){
					DrawImage(FB[i],MENU_POS_X,MENU_POS_Y+(j+1)*16,curDVDCover);
					//PrintFormat(FB[i],MENU_POS_X,MENU_POS_Y+12*16,(char*) curDVDCover);
				}
				else
					PrintFormat(FB[i],MENU_POS_X,MENU_POS_Y+12*16,"no cover image found!");

				PrintFormat( FB[i], MENU_POS_X+575, MENU_POS_Y+16*21, "%d/%d", ScrollX/10 + 1, *GameCount/10 + 1 );

				sync_after_write( (u32*)(FB[i]), FBSize );
			} break;

			case 1:
			{
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*0, "Game Region     :%s", RegionStr[DICfg->Region] );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*1, "__fwrite patch  :%s", (DICfg->Config&CONFIG_PATCH_FWRITE) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*2, "MotionPlus video:%s", (DICfg->Config&CONFIG_PATCH_MPVIDEO) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*3, "Video mode patch:%s", (DICfg->Config&CONFIG_PATCH_VIDEO) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*4, "Error skipping  :%s", (DICfg->Config&CONFIG_DUMP_ERROR_SKIP) ? "On" : "Off" );

				PrintFormat( FB[i], MENU_POS_X+80, 104+16*6, "save config" );
			//	PrintFormat( FB[i], MENU_POS_X+80, 104+16*7, "recreate game info cache" );

				PrintFormat( FB[i], MENU_POS_X+60, 40+64+16*PosX, "-->");
				sync_after_write( (u32*)(FB[i]), FBSize );
			} break;

			case 2:
			{
				if( DVDStatus == 0 )
				{
					DVDEjectDisc();

					DVDInit();
					DVDStatus = 1;					
				}

				if( DIP_COVER & 1 )
				{
					PrintFormat( FB[i], MENU_POS_X+80, 104+16*0, "Please insert a disc" );

					DVDStatus = 1;
					DVDError  = 0;

				} else {

					if( DVDError )
					{
						switch(DVDError)
						{
							default:
								PrintFormat( FB[i], MENU_POS_X+80, 104+16*0, "DVDCommand failed with:%08X", DVDError );	
							break;
						}					
					} else {
						switch( DVDStatus )
						{
							case 1:
							{
								DVDLowReset();

								s32 r = DVDLowReadDiscID( (void*)0 );
								if( r != DI_SUCCESS )
								{
									dbgprintf("DVDLowReadDiscID():%d\n", r );
									DVDError = DVDLowRequestError();
									dbgprintf("DVDLowRequestError():%X\n", DVDError );
								} else {

									hexdump( (void*)0, 0x20);

									//Detect disc type
									if( *(u32*)0x18 == 0x5D1C9EA3 )
									{
										//try a read outside the normal single layer area
										r = DVDLowRead( (char*)0x01000000, 0x172A33100LL, 0x8000 );
										if( r != 0 )
										{
											r = DVDLowRequestError();
											if( r == 0x052100 )
												DVDType = 2;
											else 
												DVDError = r;
										} else {
											DVDType = 3;
										}										

									} else if( *(u32*)0x1C == 0xC2339F3D ) {
										DVDType = 1;
									}

									DVDStatus = 2;

									r = DVDLowRead( (char*)(0x01000000+READSIZE), 0x20, 0x40 );
									if( r == DI_SUCCESS )
										DVDTitle = (char*)(0x01000000+READSIZE);

									DVDTimer = *(vu32*)0x0d800010;
								}
							} break;
							case 2:
							{
								switch(DVDType)
								{
									case 1:
										PrintFormat( FB[i], MENU_POS_X, 104+16*0, "Press A to dump: %.25s(GC)", DVDTitle );
									break;
									case 2:
										PrintFormat( FB[i], MENU_POS_X, 104+16*0, "Press A to dump: %.25s(WII-SL)", DVDTitle );
									break;
									case 3:
										PrintFormat( FB[i], MENU_POS_X, 104+16*0, "Press A to dump: %.25s(WII-DL)", DVDTitle );
									break;
									default:
										PrintFormat( FB[i], MENU_POS_X, 104+16*0, "UNKNOWN disc type!");
									break;
								}
								
							} break;
							case 3:		// Setup ripping 
							{
								DiscName = (char*)malloca( 64, 32 );

								switch( DVDType )
								{
									case 1:	// GC		0x57058000,44555
									{
										_sprintf( DiscName, "/%.6s.gcm", (void*)0 );

										DVDSectorSize = 44555;
										DVDStatus = 4;
									} break;
									case 2:	// WII-SL	0x118240000, 143432
									{
										_sprintf( DiscName, "/%.6s_0.iso", (void*)0 );

										DVDSectorSize = 143432;
										DVDStatus = 4;
									} break;
									case 3:	// WII-DL	0x1FB4E0000, 259740 
									{
										_sprintf( DiscName, "/%.6s_0.iso", (void*)0 );

										DVDSectorSize = 259740;
										DVDStatus = 4;
									} break;
								}

								if( DVDStatus == 4 )
								{
									DVDHandle = DVDOpen( DiscName );
									if( DVDHandle < 0 )
									{
										dbgprintf("ES:DVDOpen():%d\n", DVDHandle );
										DVDError = DI_FATAL|(DVDHandle<<16);
										break;
									}

									DVDErrorRetry = 0;
									DVDBuffer = (char*)0x01000000;
									memset32( DVDBuffer, 0, READSIZE );
									DVDTimer = *(vu32*)0x0d800010;
									DVDTimeStart = DVDTimer;
								}

							} break;
							case 4:
							{
								PrintFormat( FB[i], MENU_POS_X, 104+16*0, "Dumping:  %.25s", DVDTitle );
								if( DVDSpeed / 1024 / 1024 )
									PrintFormat( FB[i], MENU_POS_X, 104+16*2, "Speed:    %u.%uMB/s ", DVDSpeed / 1024 / 1024, (DVDSpeed / 1024) % 1024 );
								else
									PrintFormat( FB[i], MENU_POS_X, 104+16*2, "Speed:    %uKB/s ", DVDSpeed / 1024 );
								PrintFormat( FB[i], MENU_POS_X, 104+16*3, "Progress: %u%%", DVDOffset*100 / DVDSectorSize );
								PrintFormat( FB[i], MENU_POS_X, 104+16*4, "Time left:%02d:%02d:%02d", DVDTimeLeft/3600, (DVDTimeLeft/60)%60, DVDTimeLeft%60%60 );

								if( (DVDOffset%16) == 0 )
									dbgprintf("\rES:Dumping:%s %08X/%08X", DiscName, DVDOffset, DVDSectorSize );

								if( i == 0 )
								{
									if( (*(vu32*)0x0d800010 - DVDTimer) / 1897704 > 0 )
									{	
										DVDSpeed	= ( DVDOffset - DVDOldOffset ) * READSIZE;
										DVDTimeLeft = ( DVDSectorSize - DVDOffset ) / ( DVDSpeed / READSIZE );

										DVDOldOffset=  DVDOffset;
										DVDTimer	= *(vu32*)0x0d800010;
									}

									s32 ret = DVDLowRead( DVDBuffer, (u64)DVDOffset * READSIZE, READSIZE );
									if( ret != 0 )
									{
										DVDError = DVDLowRequestError();
										if( DVDError == 0x0030200 )
										{
											if( DVDErrorSkip == 1 )
											{
												memset32( DVDBuffer, 0, READSIZE );
											} else {
												if( DICfg->Config & CONFIG_DUMP_ERROR_SKIP )
												{
													dbgprintf("\nES:Enabled error skipping\n");
													DVDErrorSkip = 1;
												} else {
													dbgprintf("\nES:DVDLowRead():%d\n", ret );
													dbgprintf("ES:DVDError:%X\n", DVDError );
													break;
												}
											}

										} else if ( DVDError == 0x0030201 && DVDErrorRetry < 5 ) {

											dbgprintf("\nES:DVDLowRead failed:%x Retry:%d\n", DVDError, DVDErrorRetry );
											DVDError = 0;
											DVDErrorRetry++;
											DVDOffset--;
											continue;

										} else {

											dbgprintf("\nES:DVDLowRead():%d\n", ret );
											dbgprintf("ES:DVDError:%X\n", DVDError );
											break;

										}
									}

									ret = DVDWrite( DVDHandle, DVDBuffer, READSIZE );
									if( ret != READSIZE )
									{
										dbgprintf("\nES:DVDWrite():%d\n", ret );
										DVDError = DI_FATAL|(ret<<16);
										break;
									}

									DVDOffset++;
									if( DVDOffset == 131071 )
									{
										DVDClose( DVDHandle );

										_sprintf( DiscName, "/%.6s_1.iso", (void*)0 );
										DVDHandle = DVDOpen( DiscName );
										if( DVDHandle < 0 )
										{
											dbgprintf("ES:DVDOpen():%d\n", DVDHandle );
											DVDError = DI_FATAL|(DVDHandle<<16);
											break;
										}

									}
									if( DVDOffset >= DVDSectorSize )
									{
										DVDClose( DVDHandle );
										free( DiscName );
										DVDStatus = 5;
										break;
									}
								}

							} break;
							case 5:
							{
								PrintFormat( FB[i], MENU_POS_X, 104+16*0, "Dumped:   %.25s", DVDTitle );
							} break;
						}
					}
				}

			} break;
			case 3:
			{
				memcpy( FB[i], PICBuffer, FBSize );
			} break;
			default:
			{
				MenuType = 0;
				ShowMenu = 0;
			} break;
		}
	}
}

void LoadDVDCover(){
	if (curDVDCover != NULL)
		free(curDVDCover);
	curDVDCover = NULL;
	
	if (DICfg == NULL || PosX + ScrollX >= DICfg->Gamecount)
		return;

	char* imgPathBuffer = (char*)malloca(160,32);
	_sprintf( imgPathBuffer, "/sneek/covers/%s.raw", DICfg->GameInfo[PosX+ScrollX]);
	curDVDCover = LoadImage(imgPathBuffer);
	if (curDVDCover == NULL){
		_sprintf( imgPathBuffer, "/sneek/covers/%s.bmp", DICfg->GameInfo[PosX+ScrollX]);
		curDVDCover = LoadImage(imgPathBuffer);
	}
	free(imgPathBuffer);
}

void SMenuReadPad ( void )
{
	memcpy( &GCPad, (u32*)0xD806404, sizeof(u32) * 2 );

	if( ( GCPad.Buttons & 0x1F3F0000 ) == 0 && ( *WPad & 0x0000FFFF ) == 0 )
	{
		SLock = 0;
		return;
	}

	if( SLock == 0 )
	{
		if( GCPad.Start || (*WPad&WPAD_BUTTON_1) )
		{
			ShowMenu = !ShowMenu;
			if(ShowMenu)
			{
				if( DICfg == NULL )
				{
					DVDGetGameCount( GameCount );

                    DICfg = (DIConfig *)malloca( *GameCount * 0x80 + 0x10, 32 );
                    DVDReadGameInfo( 0, *GameCount * 0x80 + 0x10, DICfg );
				}
				if (MenuType == 0){
					LoadDVDCover();
				}
			}
			SLock = 1;
		}

		if( !ShowMenu )
			return;
		
		if( (GCPad.B || (*WPad&WPAD_BUTTON_B) ) && SLock == 0 )
		{
			if( MenuType == 3 )
				free( PICBuffer );

			if( MenuType == 0 ){
				if (curDVDCover != NULL)
					free(curDVDCover);
				curDVDCover = NULL;
				ShowMenu = 0;
			}

			MenuType = 0;

			PosX	= 0;
			ScrollX	= 0;
			SLock	= 1;
		}

		if( (GCPad.X || (*WPad&WPAD_BUTTON_PLUS) ) && SLock == 0 && MenuType != 1 )
		{
			if (curDVDCover != NULL)
				free(curDVDCover);
			curDVDCover = NULL;

			MenuType = 1;

			PosX	= 0;
			ScrollX	= 0;
			SLock	= 1;
		}

		if( (GCPad.Y || (*WPad&WPAD_BUTTON_MINUS) ) && SLock == 0 && MenuType != 2 )
		{
			if (curDVDCover != NULL)
				free(curDVDCover);
			curDVDCover = NULL;

			MenuType = 2;

			PosX	= 0;
			ScrollX	= 0;
			SLock	= 1;
		}

		if( (GCPad.Z || (*WPad&WPAD_BUTTON_2) ) && SLock == 0 && MenuType != 3 )
		{
			MenuType = 3;

			PICSize = 0;
			PICNum = 0;

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

		switch( MenuType )
		{
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
					if( PosX ){
						PosX--;
						LoadDVDCover();
					}
					else if( ScrollX )
					{
						ScrollX--;
						LoadDVDCover();
					}

					SLock = 1;
				} else if( GCPad.Down || (*WPad&WPAD_BUTTON_DOWN) )
				{
					if( PosX >= 9 )
					{
						if( PosX+ScrollX+1 < *GameCount )
						{
							ScrollX++;
							LoadDVDCover();
						}
					} else if ( PosX+ScrollX+1 < *GameCount ){
						PosX++;
						LoadDVDCover();
					}

					SLock = 1;
				} else if( GCPad.Right || (*WPad&WPAD_BUTTON_RIGHT) )
				{
					if( ScrollX/10*10 + 10 < *GameCount )
					{
						PosX	= 0;
						ScrollX = ScrollX/10*10 + 10;
						LoadDVDCover();
					} else {
						if (PosX || ScrollX){
							LoadDVDCover();
						}
						PosX	= 0;
						ScrollX	= 0;
					}

					SLock = 1; 
				} else if( GCPad.Left || (*WPad&WPAD_BUTTON_LEFT) )
				{
					if( ScrollX/10*10 - 10 > 0 )
					{
						PosX	= 0;
						ScrollX-= 10;
					} else {
						PosX	= 0;
						ScrollX	= 0;
					}
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
						} break;
						case 1:
						{
							DICfg->Config ^= CONFIG_PATCH_FWRITE;
						} break;
						case 2:
						{
							DICfg->Config ^= CONFIG_PATCH_MPVIDEO;
						} break;
						case 3:
						{
							DICfg->Config ^= CONFIG_PATCH_VIDEO;
						} break;
						case 5:
						{
							DICfg->Config ^= CONFIG_DUMP_ERROR_SKIP;
						} break;
						case 6:
						{
							DVDWriteDIConfig( DICfg );
						} break;
					}
					SLock = 1;
				}
				if( GCPad.Up || (*WPad&WPAD_BUTTON_UP) )
				{
					if( PosX )
						PosX--;
					else
						PosX = 6;

					if( PosX == 5 )
						PosX  = 4;

					SLock = 1;
				} else if( GCPad.Down || (*WPad&WPAD_BUTTON_DOWN) )
				{
					if( PosX >= 6 )
					{
						PosX=0;
					} else 
						PosX++;

					if( PosX == 5 )
						PosX  = 6;

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
						} break;
						case 1:
						{
							DICfg->Config ^= CONFIG_PATCH_FWRITE;
						} break;
						case 2:
						{
							DICfg->Config ^= CONFIG_PATCH_MPVIDEO;
						} break;
						case 3:
						{
							DICfg->Config ^= CONFIG_PATCH_VIDEO;
						} break;
						case 4:
						{
							DICfg->Config ^= CONFIG_DUMP_ERROR_SKIP;
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
						} break;
						case 1:
						{
							DICfg->Config ^= CONFIG_PATCH_FWRITE;
						} break;
						case 2:
						{
							DICfg->Config ^= CONFIG_PATCH_MPVIDEO;
						} break;
						case 3:
						{
							DICfg->Config ^= CONFIG_PATCH_VIDEO;
						} break;
						case 4:
						{
							DICfg->Config ^= CONFIG_DUMP_ERROR_SKIP;
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
						DVDStatus = 3;
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
}
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
			case 1:
			{
				PrintFormat( FB[i], MENU_POS_X, 40, "SNEEK+DI %s  Cheater!!!", __DATE__ );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*0, "Search value..." );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*1, "RAM viewer...(NYI)" );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*2, "RAM dumper...(NYI)" );

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
		if( (*WPad & ( WPAD_BUTTON_B | WPAD_BUTTON_1 )) == ( WPAD_BUTTON_B | WPAD_BUTTON_1 ) )
		{
			ShowMenu = !ShowMenu;
			SLock = 1;
		}

		if( (*WPad & ( WPAD_BUTTON_HOME | WPAD_BUTTON_2 )) == ( WPAD_BUTTON_HOME | WPAD_BUTTON_2 ) )
		{
			u8 *buf = (u8*)malloc( FBSize );
			memcpy( buf, (void*)(FB[0]), FBSize );

			dbgprintf("ES:Taking screenshot...");

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

			IOS_Write( fd, buf, FBSize );

			IOS_Close( fd );

			free( buf );
			free( str );

			dbgprintf("done\n");
			SLock = 1;
		}
	
		if( !ShowMenu )
			return;

		switch( ShowMenu )
		{
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
