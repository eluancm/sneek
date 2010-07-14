#include "SMenu.h"


u32 FrameBuffer	= 0;
u32 FBOffset	= 0;
u32 FBEnable	= 0;
u32	FBSize		= 0;
u32 *WPad		= NULL;
u32 *GameCount;

u32 ShowMenu=0;
u32 SLock=0;
s32 PosX=0,ScrollX=0;

u32 FB[MAX_FB];

u32 Freeze;
u32 value;
u32 *offset;


u32 PosValX;
u32 Hits;
u32 edit;
u32 *Offsets;

GCPadStatus GCPad;

DIConfig *DICfg;

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
    0x7C, 0xE3, 0x3B, 0x78,
	0x38, 0x87, 0x00, 0x34,
	0x38, 0xA7, 0x00, 0x38,
	0x38, 0xC7, 0x00, 0x4C, 
};

s32 SMenuFindOffsets( void *ptr, u32 size )
{
	u32 i;
	u32 r13  = 0;

	FBOffset = 0;
	FBEnable = 0;
	WPad	 = (u32*)NULL;

	for( i = 0; i < size; i+=4 )
	{
		if( *(u32*)(ptr+i) >> 16 == 0x3DA0 && r13 == 0 )
		{
			r13 = ((*(u32*)(ptr+i)) & 0xFFFF) << 16;
			r13|= (*(u32*)(ptr+i+4)) & 0xFFFF;
			dbgprintf("r13:%08X\n", i );
		}

		if( memcmp( ptr+i, VISetFB, sizeof(VISetFB) ) == 0 && FBEnable == 0 )
		{
			FBEnable = ( *(u32*)(ptr+i+sizeof(VISetFB)) );
			FBEnable = ((~FBEnable) & 0xFFFF) + 1;
			FBEnable = (r13 - FBEnable) & 0x7FFFFFF;
			dbgprintf("FBe:%08X\n", i );

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

		if( r13 && FBEnable && FBOffset && WPad )
			break;
	}

	dbgprintf("ES:r13     :%08X\n", r13 );
	dbgprintf("ES:FBEnable:%08X\n", FBEnable );
	dbgprintf("ES:FBOffset:%08X\n", FBOffset );
	dbgprintf("ES:WPad    :%p\n",	WPad );

	if( r13 && FBEnable && FBOffset && (WPad != NULL) )
		return 1;

	return 0;
}
s32 SMenuInit( u64 TitleID, u16 TitleVersion )
{
	value	= 0;
	Freeze	= 0;
	ShowMenu= 0;
	SLock	= 0;
	PosX	= 0;
	ScrollX	= 0;
	PosValX	= 0;
	Hits	= 0;
	edit	= 0;
	FB[0]	= 0;
	FB[1]	= 0;
	FB[2]	= 0;
	DICfg	= NULL;

	Offsets	  = (u32*)malloca( sizeof(u32)*MAX_HITS, 32 );
	GameCount = (u32*)malloca( sizeof(u32), 32 );

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
					
					FBSize		= 320*480*4;

					return 1;
				} break;
				case 514:	// EUR 4.3
				{
					//Disc Region free hack
					*(u32*)0x0137DE28 = 0x4800001C;
					*(u32*)0x0137E7A4 = 0x38000001;
					
					FBSize		= 320*480*4;

					return 1;
				} break;
				case 481:	// USA 4.2
				{
					//Disc Region free hack
					*(u32*)0x0137DBE8 = 0x4800001C;
					*(u32*)0x0137E43C = 0x60000000;

					FBSize		= 304*480*4;

					return 1;	
				} break;
				case 513:	// USA 4.3
				{
					FBSize		= 304*480*4;

					return 1;	
				} break;
			}
		} break;
		default:
		{
			FBSize = 320*480*4;
			return 1;
		} break;
	}
	return 0;
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

	for( i=0; i < MAX_FB; i++)
	{
		if( FB[i] == 0 )
			continue;

		PrintFormat( FB[i], MENU_POS_X, 40, "SNEEK+DI %s  Games:%d  Region:%s", __DATE__, *GameCount, RegionStr[DICfg->Region] );
		PrintFormat( FB[i], MENU_POS_X+600, MENU_POS_Y+16*21, "%d/%d", ScrollX/20 + 1, *GameCount/20 + 1 );

		switch( ShowMenu )
		{
			case 1:
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

				PrintFormat( FB[i], MENU_POS_X, 40+16, "GameRegion:%s", RegionStr[gRegion] );

				for( j=0; j<20; ++j )
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
				sync_after_write( (u32*)(FB[i]), FBSize );
			} break;

			case 2:
			{
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*0, "Game Region     :%s", RegionStr[DICfg->Region] );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*1, "__fwrite patch  :%s", (DICfg->Config&CONFIG_PATCH_FWRITE) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*2, "MotionPlus video:%s", (DICfg->Config&CONFIG_PATCH_MPVIDEO) ? "On" : "Off" );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*3, "Video mode patch:%s", (DICfg->Config&CONFIG_PATCH_VIDEO) ? "On" : "Off" );

				PrintFormat( FB[i], MENU_POS_X+80, 104+16*5, "save config" );
			//	PrintFormat( FB[i], MENU_POS_X+80, 104+16*6, "recreate game info cache" );

				PrintFormat( FB[i], MENU_POS_X+60, 40+64+16*PosX, "-->");
				sync_after_write( (u32*)(FB[i]), FBSize );
			} break;
		}
	}
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

					DICfg = (DIConfig *)malloca( *GameCount * 0x60 + 0x10, 32 );
					DVDReadGameInfo( 0, *GameCount * 0x60 + 0x10, DICfg );
				}
			}
			SLock = 1;
		}

		if( ShowMenu && (GCPad.B || (*WPad&WPAD_BUTTON_B) ) && SLock == 0 )
		{
			if(ShowMenu==1)
				ShowMenu=2;
			else
				ShowMenu=1;
			PosX=0;
			ScrollX=0;
			SLock = 1;
		}

		switch( ShowMenu )
		{
			case 1:			// Game list
			{
				if( GCPad.A || (*WPad&WPAD_BUTTON_A) )
				{
					DVDSelectGame( PosX+ScrollX );
					ShowMenu = 0;
					SLock = 1;
				}
				if( GCPad.Up || (*WPad&WPAD_BUTTON_UP) )
				{
					if( PosX )
						PosX--;
					else if( ScrollX )
					{
						ScrollX--;
					}

					SLock = 1;
				} else if( GCPad.Down || (*WPad&WPAD_BUTTON_DOWN) )
				{
					if( PosX >= 19 )
					{
						if( PosX+ScrollX+1 < *GameCount )
						{
							ScrollX++;
						}
					} else if ( PosX+ScrollX+1 < *GameCount )
						PosX++;

					SLock = 1;
				} else if( GCPad.Right || (*WPad&WPAD_BUTTON_RIGHT) )
				{
					if( ScrollX/20*20 + 20 < *GameCount )
					{
						PosX	= 0;
						ScrollX = ScrollX/20*20 + 20;
					} else {
						PosX	= 0;
						ScrollX	= 0;
					}

					SLock = 1; 
				} else if( GCPad.Left || (*WPad&WPAD_BUTTON_LEFT) )
				{
					if( ScrollX/20*20 - 20 > 0 )
					{
						PosX	= 0;
						ScrollX-= 20;
					} else {
						PosX	= 0;
						ScrollX	= 0;
					}

					SLock = 1; 
				}
			} break;
			case 2:		//SNEEK Settings
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
						PosX = 5;

					if( PosX == 4 )
						PosX  = 3;

					SLock = 1;
				} else if( GCPad.Down || (*WPad&WPAD_BUTTON_DOWN) )
				{
					if( PosX >= 5 )
					{
						PosX=0;
					} else 
						PosX++;

					if( PosX == 4 )
						PosX  = 5;

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
					}
					SLock = 1;
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
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*1, "RAM viewer..." );
				PrintFormat( FB[i], MENU_POS_X+80, 104+16*2, "RAM dumper..." );

				PrintFormat( FB[i], MENU_POS_X+80-6*3, 104+16*PosX, "-->");

			} break;
			case 2:
			{
				PrintFormat( FB[i], MENU_POS_X, 40, "SNEEK+DI %s  Search value", __DATE__ );
				PrintFormat( FB[i], MENU_POS_X, 104+16*-1, "Hits :%08X:%08X", Hits, *(vu32*)WPad );
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
	if( ( GCPad.Buttons & 0x1F3F0000 ) == 0 )
	{
		SLock = 0;
		return;
	}

	if( SLock == 0 )
	{
		if( GCPad.Start )
		{
			ShowMenu = !ShowMenu;
			SLock = 1;
			PosX  = 0;
		}
		if( GCPad.Z )
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
	
		if( ShowMenu == 0 )
			return;

		switch( ShowMenu )
		{
			case 1:
			{
				if( GCPad.A )
				{
					ShowMenu= 2;
					SLock	= 1;
					PosValX	= 0;
				}
			} break;
			case 2:
			{
				if( GCPad.A )
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
				if(  GCPad.B )
				{
					ShowMenu= 1;
					SLock	= 1;
				}
				if(  GCPad.X )
				{
					ShowMenu= 3;
					PosX	= 0;
					edit	= 0;
					SLock	= 1;
				}
				if(  GCPad.Left )
				{
					if( PosValX > 0 )
						PosValX--;
					SLock = 1;
				} else if(  GCPad.Right )
				{
					if( PosValX < 7 )
						PosValX++;
					SLock = 1;
				}
				if( GCPad.Up )
				{
					if( ((value>>((7-PosValX)<<2)) & 0xF) == 0xF )
					{
						value &= ~(0xF << ((7-PosValX)<<2));
					} else {
						value += 0x1 << ((7-PosValX)<<2);
					}
					SLock = 1;
				} else if( GCPad.Down )
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
