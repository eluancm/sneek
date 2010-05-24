#include "SMenu.h"


u32 FrameBuffer	= 0;
u32 FBOffset	= 0;
u32 FBEnable	= 0;
u32	FBSize		= 0;
u32 *WPad		= NULL;
u32 *GameCount;

u32 ShowMenu=0;
u32 SLock=0;
u32 PosX=0,ScrollX=0;

u32 FB[3];

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

s32 SMenuInit( u64 TitleID, u16 TitleVersion )
{
	ShowMenu=0;
	SLock=0;
	PosX=0;
	ScrollX=0;
	FB[0] = 0;
	FB[1] = 0;
	FB[2] = 0;
	DICfg = NULL;

	GameCount = malloca( sizeof(u32), 32 );

//Patches and SNEEK Menu
	switch( TitleID )
	{
		case 0x0000000100000002LL:
		{
			switch( TitleVersion )
			{
				case 482:
				{
					//Disc Region free hack
					*(u32*)0x0137DC90 = 0x4800001C;
					*(u32*)0x0137E4E4 = 0x60000000;

					//Disc autoboot
					//*(u32*)0x0137AD5C = 0x48000020;
					//*(u32*)0x013799A8 = 0x60000000;

					//BS2Report
					//*(u32*)0x137AEC4 = 0x481B22BC;
					
					FBOffset	= 0x01699448;
					FBEnable	= 0x01699430;
					FBSize		= 320*480*4;
					WPad		= (u32*)0x0113EFE0;

					return 1;
				} break;
				case 481:
				{
					//Disc Region free hack
					*(u32*)0x0137DBE8 = 0x4800001C;
					*(u32*)0x0137E43C = 0x60000000;

					FBOffset	= 0x016975A8;
					FBEnable	= 0x01697590;
					FBSize		= 304*480*4;
					WPad		= (u32*)0x0113EFE0;

				return 1;	
				} break;
			}
		} break;
		//case 0x0001000053423445LL:	//SMG2-USA
		//{
		//	FBOffset	= 0x007D67F8;
		//	FBEnable	= 0x007D67E0;
		//	FBSize		= 320*480*4;
		//	WPad		= (u32*)0x00750A00;

		//	return 1;
		//} break;
	}
	return 0;
}
void SMenuAddFramebuffer( void )
{
	u32 i,j,f;

	if( *(vu32*)FBEnable != 1 )
		return;

	FrameBuffer = (*(vu32*)FBOffset) & 0x7FFFFFFF;

	for( i=0; i<3; i++)
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
			FB[i] = FrameBuffer;
			dbgprintf("ES:Added new FB[%d]:%08X\n", i, FrameBuffer );
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

	for( i=0; i<3; i++)
	{
		if( FB[i] == 0 )
			continue;

		PrintFormat( FB[i], MENU_POS_X, 40, "SNEEK+DI %s  Games:%d  Region:%s", __DATE__, *GameCount, RegionStr[DICfg->Region] );

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
					} else 
						PosX++;

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
