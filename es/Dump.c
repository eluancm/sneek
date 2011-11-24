#include "Dump.h"

u32 DVDStatus = 0;
u32 DVDType = 0;
u32 DVDError=0;
u32 DVDErrorSkip=0;
u32 DVDErrorRetry=0;
double DVDTimer = 0;
u32 DVDTimeStart = 0;
u32 DVDSectorSize = 0;
u32 DVDOffset = 0;
u32 DVDOldOffset = 0;
u32 DVDSpeed = 0;
u64 DVDWrote = 0;
u32 DVDTimeLeft = 0;
s32 DVDHandle = 0;
u32 Wrote=0;
u32 *Key;

char *DiscName	= (char*)NULL;
char *DVDTitle	= (char*)NULL;
char *DVDBuffer	= (char*)NULL;

//HACK: u64 division breaks compiling due missing shit so we do it the hard way :|

u64 div64( u64 a, u64 b )
{
	u64 div = 0;
	u64 cnt = b;

	while( a > cnt )
	{
		div++;
		cnt+= b;
	}

	return div;	
}
u32 DumpFile( char *FileName, u64 Offset, u32 Size )
{
	DVDLowRead( DVDBuffer, Offset, (Size+31) & (~31) ); // align reads by 32 bytes, doesn't effect final filesize!
					
	s32 fd = DVDOpen( FileName );
	if( fd < 0 )
	{
		DVDError = DI_FATAL|(0x20<<16);
		return 2;

	} else {

		if( DVDWrite( fd, DVDBuffer, Size ) != Size )
		{
			DVDError = DI_FATAL|(0x21<<16);

			DVDClose( fd );
			return 2;
		}

		DVDWrote += Size;

		DVDClose( fd );
	}

	return 1;
}
void ExtractFile( u64 PartDataOffset, u64 FileOffset, u32 Size, char *FileName )
{	
	//HACK: u64 division breaks compiling due missing shit so we do it the hard way :|
	u32 div = (u32)(FileOffset>>10) / 31;
	u64 roffset = div * 0x8000;
	u64 soffset = FileOffset - (div * 0x7C00);
	//dbgprintf("Extracting:\"%s\" RealOffset:%x%08x RealOffset:%x%08x\n", FileName, (u32)(roffset>>32), (u32)roffset, (u32)(soffset>>32), (u32)soffset );
	
	s32 fd = DVDOpen( FileName );
	if( fd < 0 )
	{
		return;
	}

	unsigned int WriteSize = 0x7C00;

	if( soffset+Size > WriteSize )
		WriteSize = WriteSize - soffset;

	char *encdata = DVDBuffer;
	char *decdata = DVDBuffer + 0x8000;
	u32 ReadRetry = 0;

	while( Size > 0 )
	{
		if( WriteSize > Size )
			WriteSize = Size;

		if( DVDLowRead( encdata, PartDataOffset+roffset, 0x8000 ) != 0 )
		{
			dbgprintf("DVDLowRead(%04X) failed!\n", DVDLowRequestError() );
			ReadRetry++;
		} else {
			roffset += 0x8000;
			ReadRetry= 0;
		}

		aes_decrypt_( *Key, encdata + 0x3D0, encdata + 0x400, 0x7C00, decdata );
		
		Size -= DVDWrite( fd, decdata+soffset, WriteSize );
		DVDWrote += WriteSize;

		Wrote+=WriteSize;

		if( soffset )
		{
			WriteSize = 0x7C00;
			soffset = 0;
		}

		if( ReadRetry > 5 )
		{
			dbgprintf("ES:Failed to read the disc, maybe dirty?\n");
			break;
		}
	}

	DVDClose( fd );
}
void DecryptRead( u64 PartDataOffset, u64 FileOffset, u32 Size, char *Buffer )
{
	//HACK: u64 division breaks compiling due missing shit so we do it the hard way :|
	u64 div = (u32)(FileOffset>>10) / 31;

	u64 roffset = div * 0x8000;
	u64 soffset = FileOffset - (div * 0x7C00);
	//dbgprintf("Reading:%x%08x RealOffset:%x%08x RealOffset:%x%08x\n", (u32)(FileOffset>>32), (u32)FileOffset, (u32)(roffset>>32), (u32)roffset, (u32)(soffset>>32), (u32)soffset );

	u32 ReadSize = 0x7C00;
	u32 Read = 0;

	if( soffset+Size > ReadSize )
		ReadSize = ReadSize - soffset;

	char *encdata = DVDBuffer;
	char *decdata = DVDBuffer + 0x8000;
	u32 ReadRetry = 0;
		
	while(Size > 0)
	{
		if( ReadSize > Size )
			ReadSize = Size;

		if( DVDLowRead( encdata, PartDataOffset+roffset, 0x8000 ) != 0 )
		{
			dbgprintf("DVDLowRead(%04X) failed!\n", DVDLowRequestError() );
			ReadRetry++;
		} else {
			roffset += 0x8000;
			ReadRetry= 0;
		}

		aes_decrypt_( *Key, encdata + 0x3D0, encdata + 0x400, 0x7C00, decdata );

		memcpy( Buffer+Read, decdata+soffset, ReadSize );
		
		Read += ReadSize;
		Size -= ReadSize;
		if( soffset )
		{
			ReadSize = 0x7C00;
			soffset = 0;
		}

		if( ReadRetry > 5 )
		{
			dbgprintf("ES:Failed to read the disc, maybe dirty?\n");
			break;
		}
	}
}

void Asciify( char *str )
{
	int i=0;
	for( i=0; i < strlen(str); i++ )
		if( str[i] < 0x20 || str[i] > 0x7F )
			str[i] = '_';
}

u32 DumpDoTick( u32 CurrentFB )
{
	u32 i;
	static u32 GameSizeMB=0,GameSizeKB=0;

	if( DVDStatus == 0 )
	{
		DVDEjectDisc();
		DVDStatus = 1;					
	}

	if( DIP_COVER & 1 )
	{
		PrintFormat( FB[CurrentFB], MENU_POS_X+80, 104+16*0, "Please insert a disc" );

		DVDStatus = 1;
		DVDError  = 0;

	} else {

		if( DVDError )
		{
			switch(DVDError)
			{
				default:
					PrintFormat( FB[CurrentFB], MENU_POS_X+80, 104+16*0, "DVDCommand failed with:%08X", DVDError );	
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
						//dbgprintf("DVDLowReadDiscID():%d\n", r );
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

						DVDTitle = (char*)malloca( 32, 0x40 );

						r = DVDLowRead( (char*)(0x01000000+READSIZE), 0x20, 0x40 );
						if( r == DI_SUCCESS )
							strcpy( DVDTitle, (char*)(0x01000000+READSIZE) );

						DVDTimer = *(vu32*)0x0d800010;
					}
				} break;
				case 2:
				{
					switch(DVDType)
					{
						case 1:
							PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*0, "Press A to dump: %.24s(GC)", DVDTitle );
						break;
						case 2:
							PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*0, "Press A to dump: %.20s(WII-SL)", DVDTitle );
						break;
						case 3:
							PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*0, "Press A to dump: %.20s(WII-DL)", DVDTitle );
						break;
						default:
							PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*0, "UNKNOWN disc type!");
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
					PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*0, "Dumping:  %.24s", DVDTitle );
					if( DVDSpeed / 1024 / 1024 )
						PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*2, "Speed:    %u.%uMB/s ", DVDSpeed / 1024 / 1024, (DVDSpeed / 1024) % 1024 );
					else
						PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*2, "Speed:    %uKB/s ", DVDSpeed / 1024 );
					PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*3, "Progress: %u%%", DVDOffset*100 / DVDSectorSize );
					PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*4, "Time left:%02d:%02d:%02d", DVDTimeLeft/3600, (DVDTimeLeft/60)%60, DVDTimeLeft%60%60 );

					if( (DVDOffset%16) == 0 )
						dbgprintf("\rES:Dumping:%s %08X/%08X", DiscName, DVDOffset, DVDSectorSize );

					if( CurrentFB == 0 )
					{
						double Now = *(vu32*)0x0d800010;
						if( (u32)((Now-DVDTimer) * 128.0f / 243000000.0f) )	//Update values once per second
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
										//dbgprintf("\nES:DVDLowRead():%d\n", ret );
										dbgprintf("ES:DVDError:%X\n", DVDError );
										break;
									}
								}

							} else if ( DVDError == 0x0030201 && DVDErrorRetry < 5 ) {

								dbgprintf("\nES:DVDLowRead failed:%x Retry:%d\n", DVDError, DVDErrorRetry );
								DVDError = 0;
								DVDErrorRetry++;
								DVDOffset--;
								return 2;

							} else {

								//dbgprintf("\nES:DVDLowRead():%d\n", ret );
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
					PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*0, "Dumped:   %.25s", DVDTitle );
				} break;
				case 6:	// Setup EX format ripping
				{
					dbgprintf("ES:Ex format ripping started!\n");

					DVDTimer = *(vu32*)0x0d800010;
					DVDTimeStart = DVDTimer;
					DVDWrote = 0;

					char *FilePath = (char*)malloca( 256, 32 );

					DVDErrorRetry = 0;
					DVDBuffer = (char*)0x01000000;
					memset32( DVDBuffer, 0, READSIZE );

					_sprintf( FilePath, "/games/%.6s", (void*)0 );
					DVDCreateDir( FilePath );
					_sprintf( FilePath, "/games/%.6s/sys", (void*)0 );
					DVDCreateDir( FilePath );
					_sprintf( FilePath, "/games/%.6s/files", (void*)0 );
					DVDCreateDir( FilePath );

					DVDLowRead( DVDBuffer, 0x40000, 32 );	// Read partition table info

					u32 PartCount = *(vu32*)DVDBuffer;
					u32 GameOffset= 0;
					
					dbgprintf("ES:Partitions:%d\n", PartCount );
					dbgprintf("ES:Partition Info Offset:0x%08X\n", *(vu32*)(DVDBuffer+4) << 2 );
					
					DVDLowRead( DVDBuffer, *(vu32*)(DVDBuffer+4) << 2, 64 ); // Read partition info

					for( i=0; i < PartCount; ++i)
					{
						dbgprintf("ES:\t%d: Offset:0x%08X Type:%d\n", i, (((vu32*)DVDBuffer)[i*2])<<2, (((vu32*)DVDBuffer)[i*2+1])<<2 );
						if( (((vu32*)DVDBuffer)[i*2+1])<<2 == 0 )
						{
							GameOffset = (((vu32*)DVDBuffer)[i*2])<<2;
							break;
						}
					}

					dbgprintf("ES:Game parition offset: %08X\n", GameOffset );
					
					//Get TMD,TICKET,CERT and DATA offsets

					DVDLowRead( DVDBuffer, GameOffset + 0x2A0, sizeof(u32) * 8 );
					
					PartOffsets pf;

					memcpy( &pf, DVDBuffer, sizeof(PartOffsets) );					

					dbgprintf( "ES:tmd size    = %08x\n", pf.TMDSize );
					dbgprintf( "ES:tmd Offset  = %08x\n", pf.TMDOffset << 2);
					dbgprintf( "ES:cert size   = %08x\n", pf.CertSize );
					dbgprintf( "ES:cert Offset = %08x\n", pf.CertOffset << 2);
					dbgprintf( "ES:data size   = %08x\n", pf.DataSize );
					dbgprintf( "ES:data Offset = %08x\n", pf.DataOffset << 2);

					//Fist the easy parts: BOOT TMD TICKET CERT
					
				//Extract Ticket					
					_sprintf( FilePath, "/games/%.6s/ticket.bin", (void*)0 );
					DumpFile( FilePath, GameOffset, 0x2A4 );

				//Extract TMD
					_sprintf( FilePath, "/games/%.6s/tmd.bin", (void*)0 );
					DumpFile( FilePath, (pf.TMDOffset<<2) + GameOffset, pf.TMDSize );
					
				//Extract cert
					_sprintf( FilePath, "/games/%.6s/cert.bin", (void*)0 );
					DumpFile( FilePath, (pf.CertOffset<<2) + GameOffset, pf.CertSize );


					//Setup crypto
					u8 *TitleKey = (u8*)malloca( 16, 32 );
					u8 *TitleID  = (u8*)malloca( 16, 32 );

					memset32( TitleID, 0, 16 );

					DVDLowRead( DVDBuffer, GameOffset, 0x2A4 );

					memcpy( TitleKey, DVDBuffer + 0x1BF, 16 );
					memcpy( TitleID,  DVDBuffer + 0x1DC, 8 );
					
					Key = (u32*)malloca( sizeof(u32), 32 );

					CreateKey( Key, 0, 0 );
					syscall_71( *Key, 8 );
					syscall_5d( *Key, 0, 4, 1, 0, TitleID, TitleKey );

					//Dump boot.bin files
					
					_sprintf( FilePath, "/games/%.6s/sys/boot.bin", (void*)0 );
					ExtractFile( GameOffset + (pf.DataOffset << 2), 0, 0x440, FilePath );

					_sprintf( FilePath, "/games/%.6s/sys/bi2.bin", (void*)0 );
					ExtractFile( GameOffset + (pf.DataOffset << 2), 0x440, 0x2000, FilePath );
					
					char *Info = (char*)malloca( 0x20, 32 );

					//Get fst, main.dol offsets
					DecryptRead( GameOffset + (pf.DataOffset << 2), 0x420, 0x20, Info ); 

					u64 DolOffset = *(vu32*)(Info+0) << 2;
					u64 FSTOffset = *(vu32*)(Info+4) << 2;
					u32 FSTSize	  = *(vu32*)(Info+8) << 2;

					u64 DolSize	  = FSTOffset - DolOffset;
					
					_sprintf( FilePath, "/games/%.6s/sys/main.dol", (void*)0 );
					ExtractFile( GameOffset + (pf.DataOffset << 2), DolOffset, DolSize, FilePath );

					_sprintf( FilePath, "/games/%.6s/sys/fst.bin", (void*)0 );
					ExtractFile( GameOffset + (pf.DataOffset << 2), FSTOffset, FSTSize, FilePath );

					//Get apploader size
					DecryptRead( GameOffset + (pf.DataOffset << 2), 0x2440, 0x20, Info );

					u32 AppSize = *(vu32*)(Info+0x14) + *(vu32*)(Info+0x18) + 0x20;

					_sprintf( FilePath, "/games/%.6s/sys/apploader.img", (void*)0 );
					ExtractFile( GameOffset + (pf.DataOffset << 2), 0x2440, AppSize, FilePath );

					//Now dump the files
					char *FSTable = (char*)( 0x01800000 - ( (FSTSize+31) & (~31) ) ); // FST can be VERY large, also align the address as dvd can only read to 32byte bounds

					dbgprintf("ES:Fst:%p Size:%x\n", FSTable, FSTSize );

					DecryptRead( GameOffset + (pf.DataOffset << 2), FSTOffset, FSTSize, FSTable );

					u32 Entries = *(u32*)(FSTable+0x08);
					u32 CurrentOff=-1;
					u32 OldOff = 0;
					u32 FileCount=0;
					u32 FileDumpCnt=0;
					char *NameOff = (char*)(FSTable + Entries * 0x0C);
					FEntry *fe = (FEntry*)(FSTable);
					
					dbgprintf("ES:Entries:%u\n", Entries );

					u32 Entry[16];
					u32 LEntry[16];
					u32 level=0;
					int j;
					char *Path = (char*)malloca( 1024, 32 );

					//Count files in FST, Entries is folder and file count!
					for( j=0; j < Entries; ++j )
					{
						if( fe[j].Type )
							continue;

						FileCount++;
					}

					while( FileDumpCnt < FileCount )
					{
						CurrentOff=-1;

						// Find lowest offset to reduce seeking
						for( j=0; j < Entries; ++j )
						{
							if( fe[j].Type )
								continue;

							if( fe[j].FileOffset <= CurrentOff && fe[j].FileOffset > OldOff )
								CurrentOff = fe[j].FileOffset;
						}

						
						level=0;

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

								if( CurrentOff == fe[i].FileOffset )
								{
									//Do not remove!
									memset32( Path, 0, 1024 );
									_sprintf( Path, "/games/%.6s/files/", (void*)0 );

									for( j=0; j<level; ++j )
									{
										if( j )
											Path[strlen(Path)] = '/';
										memcpy( Path+strlen(Path), NameOff + fe[Entry[j]].NameOffset, strlen(NameOff + fe[Entry[j]].NameOffset ) );

										Asciify(Path);
										DVDCreateDir(Path);
									}

									if( level )
										Path[strlen(Path)] = '/';
									memcpy( Path+strlen(Path), NameOff + fe[i].NameOffset, strlen(NameOff + fe[i].NameOffset) );
									

									Asciify(Path);

									dbgprintf("ES:[%X%08X:%08u]%s\n", fe[i].FileOffset>>30, (u32)(fe[i].FileOffset<<2), fe[i].FileLength, Path + 20 );	// +20 skips "/games/<gameid>/files/"
							
									ExtractFile( GameOffset + (pf.DataOffset << 2), (u64)(fe[i].FileOffset)<<2, fe[i].FileLength, Path );
									OldOff = fe[i].FileOffset;
									FileDumpCnt++;
									break;
								}
							}
						}
					}
					

					DVDTimer = (u32)(*(vu32*)0x0d800010 - DVDTimeStart);
					DVDTimer = (u32)( DVDTimer * 128.f / 243000000.f);

					DestroyKey( *Key );

					GameSizeMB = (u32)div64(DVDWrote,1024*1024);	//Cache values, calculating it twice a frame takes too long and causes the overlaymenu to flicker
					GameSizeKB = (u32)div64(DVDWrote,1024);

					free(Key);
					free(Path);
					free(FilePath);
					free(Info);
					
					DVDStatus = 7;

				} break;
				case 7:
				{	

					PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*0, "Dumped   :  %.24s", DVDTitle );
					PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*1, "Time     :  %02u:%02u", (u32)DVDTimer/60, (u32)DVDTimer%60 );
					PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*2, "Game size:  %uMB", GameSizeMB );
					PrintFormat( FB[CurrentFB], MENU_POS_X, 104+16*3, "Speed    : ~%u.%uMB/s", GameSizeMB / (u32)DVDTimer, GameSizeKB % 1024 );
				} break;
			}
		}
	}

	return 2;
}
