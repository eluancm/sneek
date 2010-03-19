/*

SNEEK - SD-NAND/ES + DI emulation kit for Nintendo Wii

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
#include "dip.h"

static u32 error ALIGNED(32);
static u32 PartitionSize ALIGNED(32);
static u32 DIStatus ALIGNED(32);
static u32 DICover ALIGNED(32);
static u32 ChangeDisc ALIGNED(32);
static u32 Region ALIGNED(32) = 2;

static char GamePath[64];
static u32 *KeyID ALIGNED(32);


#ifdef FILEMODE

static u8 *FSTable ALIGNED(32);
u32 ApploaderSize=0;
u32 DolSize=0;
u32 DolOffset=0;
u32 FSTableSize=0;
u32 FSTableOffset=0;

u32 FCEntry=0;
FileCache FC[FILECACHE_MAX];

#else
static FIL Part;
#endif


int verbose = 0;
u32 Partition = 0;
u32 Motor = 0;
u32 Disc = 0;
u64 PartitionOffset=0;
u64 PartitionDataOffset=0;
u32 PartitionTableOffset=0;

#ifdef FILEMODE
s32 DVDSelectGame( int SlotID )
{
	FIL f;
	DIR d;
	u32 read;
	FILINFO FInfo;
	u32 count = 0;

	if( f_opendir( &d, "/games" ) != FR_OK )
	{
		return DI_FATAL;
	}

	while( f_readdir( &d, &FInfo ) == FR_OK )
	{
		if( count == SlotID )
		{
			//build path
			sprintf( GamePath, "/games/%s/", FInfo.fname );
			dbgprintf("%s\n", GamePath );

			free( FSTable );

			char *str = malloca( 128, 32 );
			sprintf( str, "%ssys/fst.bin", GamePath );

			if( f_open( &f, str, FA_READ ) != FR_OK )
			{
				return DI_FATAL;
			}

			ChangeDisc = 1;
			DICover |= 1;

			FSTable = malloca( f.fsize, 32 );
			FSTableSize = f.fsize;
			f_read( &f, FSTable, f.fsize, &read );
			f_close( &f );
		
			//Get Apploader size
			sprintf( str, "%ssys/apploader.img", GamePath );
			int fres = f_open( &f, str, FA_READ );
			if( fres != FR_OK )
			{
				DIP_Fatal("DVDSelectGame()",__LINE__,__FILE__, fres, "Failed to open apploader.img!");
				while(1);
			}

			ApploaderSize = f.fsize;
			f_close( &f );
		
			dbgprintf("DIP:apploader size:%08X\n", ApploaderSize );

			free( str );

			//create slot.bin
			if( f_open( &f, "/slot.bin", FA_CREATE_ALWAYS|FA_WRITE ) == FR_OK )
			{
				f_write( &f, &SlotID, sizeof(u32), &read );
				f_close( &f );
			}

			//Init cache
			for( count=0; count<FILECACHE_MAX; ++count )
			{
				FC[count].File.fs = NULL;
			}

			return DI_SUCCESS;
		}
		count++;
	}

	return DI_FATAL;
}
#else
s32 DVDSelectGame( int SlotID )
{
	FILINFO FInfo;
	u32 count = 0;
	DIR d;

	if( f_opendir( &d, "/games" ) != FR_OK )
	{
		return DI_FATAL;
	}

	while( f_readdir( &d, &FInfo ) == FR_OK )
	{
		if( count == SlotID )
		{
			//build path
			sprintf( GamePath, "/games/%s/", FInfo.fname );
			dbgprintf("%s\n", GamePath );

			if( Part.fs != NULL )
				f_close( &Part );

			char *str = malloca( 128, 32 );
			sprintf( str, "%spart.bin", GamePath );

			if( f_open( &Part, str, FA_READ ) != FR_OK )
			{
				return DI_FATAL;
			}

			ChangeDisc = 1;
			DICover |= 1;

			free( str );

			return DI_SUCCESS;
		}
		count++;
	}

	return DI_FATAL;
}
#endif

void DIP_Fatal( char *name, u32 line, char *file, s32 error, char *msg )
{
	dbgprintf("************FATAL ERROR************\n");
	dbgprintf("Function :%s\n", name );
	dbgprintf("line     :%d\n", line );
	dbgprintf("file     :%s\n", file );
	dbgprintf("error    :%d\n", error );
	dbgprintf("%s\n", msg );
	dbgprintf("************FATAL ERROR************\n");

	while(1);
}
int DIP_Ioctl( struct ipcmessage *msg )
{
	u8  *bufin  = (u8*)msg->ioctl.buffer_in;
	u32 lenin   = msg->ioctl.length_in;
	u8  *bufout = (u8*)msg->ioctl.buffer_io;
	u32 lenout  = msg->ioctl.length_io;
	s32 ret		= DI_FATAL;
	u32 read	= 0;
	FIL f;

	//dbgprintf("DIP:IOS_Ioctl( %d 0x%x 0x%p 0x%x 0x%p 0x%x )\n", msg->fd, msg->ioctl.command, bufin, lenin, bufout, lenout);
			
	switch(msg->ioctl.command)
	{
		case DVD_READ_GAMEINFO:
		{
			FIL f;
			DIR d;
			if( f_opendir( &d, "/games" ) != FR_OK )
			{
				ret = DI_FATAL;
				break;
			}

			FILINFO FInfo;
			u32 count = 0;
			while( f_readdir( &d, &FInfo ) == FR_OK )
			{
				if( count == *(u32*)(bufin) )
				{
					char *LPath = malloca( 128, 32 );
					//build path
					sprintf( LPath, "/games/%s/sys/boot.bin", FInfo.fname );
					dbgprintf("%s\n", LPath );

					if( f_open( &f, LPath, FA_READ ) == FR_OK )
					{
						f_read( &f, (u8*)(*(u32*)(bufin+4)), 0x100, &read );
						f_close( &f );
						ret = DI_SUCCESS;
					} else {
						ret = DI_FATAL;
					}

					free( LPath );
					break;
				}
				count++;
			}
			dbgprintf("DIP:DVDReadGameInfo(%d):%d\n", *(u32*)(bufin), ret );
		} break;
		case DVD_INSERT_DISC:
		{
			DICover &= ~1;
			ret = DI_SUCCESS;
			dbgprintf("DIP:InsertDisc(%d):%d\n", ret );
		} break;
		case DVD_EJECT_DISC:
		{
			DICover |= 1;
			ret = DI_SUCCESS;
			dbgprintf("DIP:EjectDisc(%d):%d\n", ret );
		} break;
		case DVD_GET_GAMECOUNT:	//Get Game count
		{
			DIR d;
			if( f_opendir( &d, "/games" ) != FR_OK )
			{
				ret = DI_FATAL;
				break;
			}

			FILINFO FInfo;
			u32 count = 0;
			while( f_readdir( &d, &FInfo ) == FR_OK )
			{
				count++;
			}

			*(u32*)bufout = count;
			ret = DI_SUCCESS;
			dbgprintf("DIP:GetGameCount(%d):%d\n", count, ret );
		} break;
		case DVD_SELECT_GAME:
		{
			ret = DVDSelectGame( *(u32*)(bufin) );

			dbgprintf("DIP:DVDSelectGame(%d):%d\n", *(u32*)(bufin), ret );
		} break;
		case 0x96:
		case DVD_REPORTKEY:
		{
			ret = DI_ERROR;
			error = 0x00052000;
			dbgprintf("DIP:DVDLowReportKey():%d\n", ret );
		} break;
		case 0xDD:			// 0 out
		{
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowSetMaximumRotation():%d\n", ret);
		} break;
		case 0x95:			// 0x20 out
		{
			*(u32*)bufout = DIStatus;

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowPrepareStatusRegister( %08X ):%d\n", *(u32*)bufout, ret );
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
			//dbgprintf("DIP:DVDLowPrepareCoverRegister( %08X ):%d\n", *(u32*)bufout, ret );
		} break;
		case 0x86:			// 0 out
		{
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowClearCoverInterrupt():%d\n", ret);
		} break;
		case DVD_GETCOVER:	// 0x88
		{
			*(u32*)bufout = DICover;
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowGetCoverStatus( %08X ):%d\n", *(u32*)bufout, ret );
		} break;
		case 0x89:
		{
			DICover &= ~4;
			DICover |= 2;

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowUnknownRegister():%d\n", ret );
		} break;
		case DVD_IDENTIFY:
		{
			memset( bufout, 0, 0x20 );

			*(u32*)(bufout)		= 0x00000002;
			*(u32*)(bufout+4)	= 0x20070213;
			*(u32*)(bufout+8)	= 0x41000000;


			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowIdentify():%d\n", ret);
		} break;
		case DVD_GET_ERROR:	// 0xE0
		{
			*(u32*)bufout = error; 
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowGetError( %08X ):%d\n", error, ret );
		} break;
		case 0x8E:
		{
			if( (*(u8*)(bufin+4)) == 0 )
				EnableVideo( 1 );
			else
				EnableVideo( 0 );

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowEnableVideo(%d):%d\n", *(u32*)(bufin+4), ret);
		} break;
		case DVD_LOW_READ:	// 0x71
		{
			if( *(u32*)(bufin+8) > PartitionSize )
			{
				ret = DI_ERROR;
				error = 0x00052100;

			} else {
				if( Partition )
				{
#ifdef FILEMODE
					u32 ReadOffset = (*(u32*)(bufin+8)) << 2;

					if( ReadOffset < 0x440 )
					{
						char *str = malloca( 32, 32 );
						sprintf( str, "%ssys/boot.bin", GamePath );
						if( f_open( &f, str, FA_READ ) != FR_OK )
						{
							dbgprintf("DIP:[boot.bin] Failed to open!\n" );
							ret = DI_FATAL;
						} else {
							dbgprintf("DIP:[boot.bin] Offset:%08X Size:%08X\n", ReadOffset, *(u32*)(bufin+4) );
							f_lseek( &f, ReadOffset );
							f_read( &f, bufout, *(u32*)(bufin+4), &read );

							f_lseek( &f, 0x0420 );
							f_read( &f, &DolOffset, 4, &read );
							f_lseek( &f, 0x0424 );
							f_read( &f, &FSTableOffset, 4, &read );

							DolOffset<<=2;
							FSTableOffset<<=2;

							DolSize = FSTableOffset - DolOffset;

							dbgprintf("DIP:FSTableOffset:%08X\n", FSTableOffset );
							dbgprintf("DIP:DolOffset:    %08X\n", DolOffset );
							dbgprintf("DIP:DolSize:      %08X\n", DolSize );

							f_close( &f );
							ret = DI_SUCCESS;

						}

					} else if( ReadOffset < 0x2440 )
					{
						ReadOffset -= 0x440;

						char *str = malloca( 32, 32 );
						sprintf( str, "%ssys/bi2.bin", GamePath );
						if( f_open( &f, str, FA_READ ) != FR_OK )
						{
							dbgprintf("DIP:[bi2.bin] Failed to open!\n" );
							ret = DI_FATAL;
						} else {
							dbgprintf("DIP:[bi2.bin] Offset:%08X Size:%08X\n", ReadOffset, *(u32*)(bufin+4) );
							f_lseek( &f, ReadOffset );
							f_read( &f, bufout, *(u32*)(bufin+4), &read );
							f_close( &f );
							ret = DI_SUCCESS;
						}

					} else if( ReadOffset < 0x2440+ApploaderSize )
					{
						ReadOffset -= 0x2440;

						char *str = malloca( 32, 32 );
						sprintf( str, "%ssys/apploader.img", GamePath );
						if( f_open( &f, str, FA_READ ) != FR_OK )
						{
							dbgprintf("DIP:[apploader.img] Failed to open!\n" );
							ret = DI_FATAL;
						} else {
							dbgprintf("DIP:[apploader.img] Offset:%08X Size:%08X\n", ReadOffset, *(u32*)(bufin+4) );
							f_lseek( &f, ReadOffset );
							f_read( &f, bufout, *(u32*)(bufin+4), &read );
							f_close( &f );
							ret = DI_SUCCESS;
						}
					} else if( ReadOffset < DolOffset + DolSize )
					{
						ReadOffset -= DolOffset;

						char *str = malloca( 32, 32 );
						sprintf( str, "%ssys/main.dol", GamePath );
						if( f_open( &f, str, FA_READ ) != FR_OK )
						{
							dbgprintf("DIP:[main.dol] Failed to open!\n" );
							ret = DI_FATAL;
						} else {
							dbgprintf("DIP:[main.dol] Offset:%08X Size:%08X\n", ReadOffset, *(u32*)(bufin+4) );
							f_lseek( &f, ReadOffset );
							f_read( &f, bufout, *(u32*)(bufin+4), &read );
							f_close( &f );
							ret = DI_SUCCESS;
						}
					} else if( ReadOffset < FSTableOffset+FSTableSize )
					{
						ReadOffset -= FSTableOffset;

						char *str = malloca( 32, 32 );
						sprintf( str, "%ssys/fst.bin", GamePath );
						if( f_open( &f, str, FA_READ ) != FR_OK )
						{
							dbgprintf("DIP:[fst.bin] Failed to open!\n" );
							ret = DI_FATAL;
						} else {
							dbgprintf("DIP:[fst.bin] Offset:%08X Size:%08X\n", ReadOffset, *(u32*)(bufin+4) );
							f_lseek( &f, ReadOffset );
							f_read( &f, bufout, *(u32*)(bufin+4), &read );
							f_close( &f );
							ret = DI_SUCCESS;
						}
					} else {

						u32 i,j;

						//try cache first!
						for( i=0; i < FILECACHE_MAX; ++i )
						{
							if( FC[i].File.fs == NULL )
								continue;

							if( ReadOffset >= FC[i].Offset && ReadOffset < FC[i].Offset + FC[i].Size )
							{
								//dbgprintf("DIP:[Cache:%d] Offset:%08X Size:%08X\n", i, ((*(u32*)(bufin+8))<<2)-(FC[i].Offset), ((*(u32*)(bufin+4))+31)&(~31) );
								f_lseek( &FC[i].File, ((*(u32*)(bufin+8))<<2)-(FC[i].Offset) );
								f_read( &FC[i].File, bufout, ((*(u32*)(bufin+4))+31)&(~31), &read );
								ret = DI_SUCCESS;
								break;
							}
						}

						if( i < FILECACHE_MAX )
							break;

						//The fun part!
		
						u32 Entries = *(u32*)(FSTable+0x08);
						u32 NameOff = Entries * 0x0C;
						FEntry *fe = (FEntry*)(FSTable);

						char *Path=NULL;
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
								//printf("[%03X]Entering:\"%s\" Level:%d leave:%04X\n", i, buffer + NameOff + swap24( fe[i].NameOffset ), level, swap32( fe[i].NextOffset ) );
								Entry[level] = i;
								LEntry[level++] = fe[i].NextOffset;
								if( level > 15 )	// something is wrong!
									break;
							} else {
								if( ReadOffset >= (fe[i].FileOffset << 2) && ReadOffset < (fe[i].FileOffset << 2) + fe[i].FileLength )
								{
									//dbgprintf("Found Entry:%X file:%s FCEntry:%d\n", i,  FSTable + NameOff + fe[i].NameOffset, FCEntry );

									Path = malloca( 1024, 32 );
									memset( Path, 0, 1024 );
									sprintf( Path, "%sfiles/", GamePath );

									for( j=0; j<level; ++j )
									{
										if( j )
											Path[strlen(Path)] = '/';
										memcpy( Path+strlen(Path), FSTable + NameOff + fe[Entry[j]].NameOffset, strlen(FSTable + NameOff + fe[Entry[j]].NameOffset ) );
									}
									if( level )
										Path[strlen(Path)] = '/';
									memcpy( Path+strlen(Path), FSTable + NameOff + fe[i].NameOffset, strlen(FSTable + NameOff + fe[i].NameOffset) );

									if( FCEntry >= FILECACHE_MAX )
										FCEntry = 0;

									if( FC[FCEntry].File.fs != NULL )
										f_close( &FC[FCEntry].File );

									if( f_open( &FC[FCEntry].File, Path, FA_READ ) != FR_OK )
									{
										dbgprintf("DIP:[%s] Failed to open!\n", Path );
										error = 0x031100;
										ret = DI_FATAL;
									} else {

										FC[FCEntry].Size = fe[i].FileLength;
										FC[FCEntry].Offset = (u64)(fe[i].FileOffset)<<2;

										dbgprintf("DIP:[%s] Offset:%08X Size:%08X\n", Path, ((*(u32*)(bufin+8))<<2)-(fe[i].FileOffset << 2), ((*(u32*)(bufin+4))+31)&(~31) );
										f_lseek( &FC[FCEntry].File, ((*(u32*)(bufin+8))<<2)-(fe[i].FileOffset << 2) );
										f_read( &FC[FCEntry].File, bufout, ((*(u32*)(bufin+4))+31)&(~31), &read );
										
										FCEntry++;

										ret = DI_SUCCESS;
									}

									free( Path );
									break;
								}
							}
						}
					}
#else
					s32 r=0;
					r = f_lseek( &f, ((*(u32*)(bufin+8))<<2) );
					if( r != FR_OK )
					{
						dbgprintf("f_lseek():%d\n", r);
						ret = DI_FATAL;
					} else {
						r = f_read( &f, bufout, ((*(u32*)(bufin+4))+31)&(~31), &read );
						if( r != FR_OK )
						{
							dbgprintf("f_read():%d\n", r);
							ret = DI_FATAL;
						} else {
							if( read < *(u32*)(bufin+4) )
							{
								dbgprintf("f_read(): %d < %d\n", read < *(u32*)(bufin+4));
								ret = DI_FATAL;
							} else
								ret = DI_SUCCESS;
						}
					}
#endif
				} else {
					DIP_Fatal("DVDLowread", __LINE__, __FILE__, 64, "Partition not opened!");
				}
			}

			if( ret != DI_SUCCESS )
				dbgprintf("DIP:DVDLowRead( %08X, %08X, %p ):%d\n", *(u32*)(bufin+8), *(u32*)(bufin+4), bufout, ret );
		} break;
		case DVD_READ_UNENCRYPTED:
		{
			if( *(u32*)(bufin+8) > 0x46090000 )
			{
				ret = DI_ERROR;
				error = 0x00052100;

			} else {
				switch( *(u32*)(bufin+8) )
				{
#ifdef FILEMODE
					case 0x00:
					case 0x08:		// 0x20
					{
						char *str = malloca( 62, 32 );
						sprintf( str, "%ssys/boot.bin", GamePath );

						if( f_open( &f, str, FA_READ ) == FR_OK )
						{
							
							f_lseek( &f, (*(u32*)(bufin+8)) << 2 );

							if( f_read( &f, bufout, *(u32*)(bufin+4), &read ) != FR_OK )
								ret = DI_FATAL;
							else 
								ret = DI_SUCCESS;
							f_close( &f );
						} else {
							ret = DI_FATAL;
						}
						free( str );
					} break;
#else
					case 0x00:
					case 0x08:		// 0x20
					{
						f_lseek( &Part, (*(u32*)(bufin+8)) << 2 );
						if( f_read( &Part, bufout, *(u32*)(bufin+4), &read ) != FR_OK )
							ret = DI_FATAL;
						else 
							ret = DI_SUCCESS;
					} break;
#endif
					case 0x10000:		// 0x40000
					{
						memset( bufout, 0, lenout );

						*(u32*)(bufout)		= 1;				// one partition
						*(u32*)(bufout+4)	= 0x40020>>2;		// partition table info

						ret = DI_SUCCESS;
					} break;
					case 0x10008:		// 0x40020
					{
						memset( bufout, 0, lenout );

						*(u32*)(bufout)		= 0x03E00000;		// partition offset
						*(u32*)(bufout+4)	= 0x00000000;		// partition type

						ret = DI_SUCCESS;
					} break;
					case 0x00013800:		// 0x4E000
					{
						memset( bufout, 0, lenout );

						*(u32*)(bufout)			= Region;			// 0 = JAP, 1 = USA, 2 = EUR, 4 = KOR
						*(u32*)(bufout+0x1FFC)	= 0xC3F81A8E;

						dbgprintf("DIP:Region:%d\n", Region );

						ret = DI_SUCCESS;
					} break;
					default:
						dbgprintf("DIP:unexpected read offset!\n");
						dbgprintf("DIP:DVDLowUnencryptedRead( %08X, %08X, %p ):%d\n", *(u32*)(bufin+8), *(u32*)(bufin+4), bufout, ret );
						while(1);
					break;
				}
			}

			dbgprintf("DIP:DVDLowUnencryptedRead( %08X, %08X, %p ):%d\n", *(u32*)(bufin+8), *(u32*)(bufin+4), bufout, ret );
		} break;
		case DVD_READ_DISCID:
		{
#ifdef FILEMODE
			char *str = malloca( 32, 32 );
			sprintf( str, "%ssys/boot.bin", GamePath );
			if( f_open( &f, str, FA_READ ) != FR_OK )
			{
				ret = DI_FATAL;
			} else {
				f_read( &f, bufout, lenout, &read );
				f_close( &f );
				free( str );
				ret = DI_SUCCESS;

				//Starlet can only access PPC memory in 32bit read/writes
			//	*(vu32*)bufout = ((*(vu32*)bufout) & 0xFFFFFF00 ) | 'P';
			//	sync_after_write( bufout, lenout );
			}
#else
			u8 *data = malloca( (lenout+31)&(~31), 0x40 );

			f_lseek( &Part, 0 );
			ret = f_read( &Part, data, lenout, &read );
			if( ret != FR_OK )
			{
				dbgprintf("fail:%d\n", ret );
				ret = DI_FATAL;
			} else {
				ret = DI_SUCCESS;
				memcpy( bufout, data, lenout );
			}
			
			free( data );

#endif
			hexdump( bufout, lenout );

			dbgprintf("DIP:DVDLowReadDiscID(%p):%d\n", bufout, ret );
		} break;
		case DVD_RESET:
		{
			DIStatus = 0x00;
			DICover  = 0x00;
			Partition = 0;

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowReset( %d ):%d\n", *(u32*)(bufin+4), ret);
		} break;
		case DVD_SET_MOTOR:
		{
			Motor = 0;
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDStopMotor(%d,%d):%d\n", *(u32*)(bufin+4), *(u32*)(bufin+8), ret);
		} break;
		case DVD_CLOSE_PARTITION:
		{
			Partition=0;
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowClosePartition():%d\n", ret );
		} break;
		case DVD_READ_BCA:
		{
			memset32( (u32*)bufout, 0, 0x40 );
			*(u32*)(bufout+0x30) = 0x00000001;

			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowReadBCA():%d\n", ret );
		} break;
		case DVD_LOW_SEEK:
		{
			ret = DI_SUCCESS;
			dbgprintf("DIP:DVDLowSeek():%d\n", ret );
		} break;
		default:
			hexdump( bufin, lenin );
			dbgprintf("DIP:Unknown IOS_Ioctl( %d 0x%x 0x%p 0x%x 0x%p 0x%x )\n", msg->fd, msg->ioctl.command, bufin, lenin, bufout, lenout);
			MessageQueueAck( (void *)msg, DI_FATAL);
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
	u32 InCount		= msg->ioctlv.argc_in;
	u32 OutCount	= msg->ioctlv.argc_io;
	vector *v		= (vector*)(msg->ioctlv.argv);
	s32 ret			= DI_FATAL;
	u32 read		= 0;

	switch(msg->ioctl.command)
	{
		case DVD_OPEN_PARTITION:
		{
			if( Partition == 1 )
			{
				DIP_Fatal( "DVDLowOpenPartition", __LINE__, __FILE__, 0, "Partition already open!");
			}

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

			FIL fp;
			if( f_open( &fp, str, FA_READ ) == FR_OK )
			{
				TIK = (u8*)halloca( 0x2a4, 32 );
				if( f_read( &fp, TIK, 0x2a4, &read ) != FR_OK )
				{
					ret = DI_FATAL;
					f_close( &fp );
					break;
				}

				((u32*)buffer)[0x04] = (u32)TIK;			//0x10
				((u32*)buffer)[0x05] = 0x2a4;				//0x14
				sync_before_read(TIK, 0x2a4);

				f_close( &fp );
			} else {
				dbgprintf("DIP:Could not open:\"%s\"\n", str );
				ret = DI_FATAL;
				break;

			}
			sprintf( str, "%stmd.bin", GamePath );

			if( f_open( &fp, str, FA_READ ) == FR_OK )
			{
				u32 asize = (fp.fsize+31)&(~31);
				TMD = (u8*)halloca( asize, 32 );
				memset( TMD, 0, asize );
				if( f_read( &fp, TMD, fp.fsize, &read ) != FR_OK )
				{
					ret = DI_FATAL;
					f_close( &fp );
					break;
				} 

				((u32*)buffer)[0x06] = (u32)TMD;		//0x18
				((u32*)buffer)[0x07] = fp.fsize;		//0x1C
				sync_before_read(TMD, asize);

				PartitionSize = (u32)((*(u64*)(TMD+0x1EC)) >> 2 );
				dbgprintf("DIP:Set partition size to:%08X%08X\n", (PartitionSize>>29)&0xFFFFFFFF , (((u64)PartitionSize)<<2)&0xFFFFFFFF );

				if( v[3].data != NULL )
				{
					//TMD[0x0193] = 'P';
					//hexdump( TMD+0x18C, 0x20 ); 

					memcpy( v[3].data, TMD, fp.fsize );
					sync_after_write( v[3].data, fp.fsize );
				}

				f_close( &fp );
			} else {
				dbgprintf("DIP:Could not open:\"%s\"\n", str );
				ret = DI_FATAL;
				break;
			}

			sprintf( str, "%scert.bin", GamePath );
			if( f_open( &fp, str, FA_READ ) == FR_OK )
			{
				CRT = (u8*)halloca( 0xA00, 32 );
				if( f_read( &fp, CRT, 0xA00, &read ) != FR_OK )
				{
					ret = DI_FATAL;
					f_close( &fp );
					break;
				}

				((u32*)buffer)[0x00] = (u32)CRT;		//0x00
				((u32*)buffer)[0x01] = 0xA00;			//0x04
				sync_before_read(CRT, 0xA00);

				f_close( &fp );
			} else {
				dbgprintf("DIP:Could not open:\"%s\"\n", str );
				ret = DI_FATAL;
				break;
			}

			hfree( str );

			KeyID = (u32*)malloca( sizeof( u32 ) , 32 );


			((u32*)buffer)[0x02] = (u32)NULL;			//0x08
			((u32*)buffer)[0x03] = 0;					//0x0C

			//out
			((u32*)buffer)[0x08] = (u32)KeyID;			//0x20
			((u32*)buffer)[0x09] = 4;					//0x24
			((u32*)buffer)[0x0A] = (u32)hashes;			//0x28
			((u32*)buffer)[0x0B] = 20;					//0x2C

			sync_before_read(buffer, 0x40);

			s32 ESHandle = IOS_Open("/dev/es", 0 );

			s32 r = IOS_Ioctlv( ESHandle, 0x1C, 4, 2, buffer );
			if( r < 0)
				dbgprintf("ios_ioctlv( %d, %02X, %d, %d, %p ):%d\n", ESHandle, 0x1C, 4, 2, buffer, r );

			IOS_Close( ESHandle );

			*(u32*)(v[4].data) = 0x00000000;

			free( KeyID );

			hfree( buffer );
			hfree( TIK );
			hfree( CRT );
			hfree( TMD );

			if( r < 0 )
			{
				DIP_Fatal( "DVDLowOpenPartition", __LINE__, __FILE__, r, "ios_ioctlv() failed!");
			}
			
			ret = DI_SUCCESS;

			Partition=1;

			dbgprintf("DIP:DVDOpenPartition(0x%08X):%d\n", *(u32*)(v[0].data+4), ret );

		} break;
		default:
			dbgprintf("DIP:IOS_Ioctlv( %d, 0x%x 0x%x 0x%x 0x%p )\n", msg->fd, msg->ioctl.command, InCount, OutCount, msg->ioctlv.argv );
			MessageQueueAck( (void *)msg, DI_FATAL);
			while(1);
		break;
	}

	//Reset errror after a successful command
	if( ret == DI_SUCCESS )
		error = 0;

	return ret;
}



