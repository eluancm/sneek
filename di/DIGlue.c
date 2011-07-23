#include "DIGlue.h"

u32 FSMode;

DIR d;
FILINFO FInfo;
FATFS fatfs;
FIL *FHandle;

s32 FSHandle;
s32 *Nhandle;
char *Entries;
char *CEntry;
u32 FirstEntry;
u32 *EntryCount;


s32 ISFS_IsUSB( void )
{
	s32 fd = IOS_Open( "/dev/fs", 0 );
	s32 ret = IOS_Ioctl( fd, ISFS_IS_USB, NULL, 0, NULL, 0 );
	IOS_Close( fd );

	return ret;
}

void DVDInit( void )
{
	int i=0;
	s32 fres = FR_DISK_ERR;
	int MountFail=0;

	// Probe for UNEEK
	if( ISFS_IsUSB() == FS_ENOENT2 )
	{
		dbgprintf("DIP:Found FS-SD\n");
		FSMode = SNEEK;
		
		HardDriveConnected = 0;

		while(!HardDriveConnected)
		{
			while(1)
			{
				fres = f_mount(0, &fatfs );
				dbgprintf("DIP:f_mount():%d\n", fres );
				if( fres == FR_OK )
					break;
				else
					MountFail++;

				if( MountFail == 10 )
				{
					dbgprintf("DIP:too much fail! looping now!\n");
					while(1);
				}

				udelay(500000);
			}

			//try to open a file, it doesn't have to exist, just testing if FS works
			FIL f;
			fres = f_open( &f, "/randmb.in", FA_READ|FA_OPEN_EXISTING );
			switch(fres)
			{
				case FR_OK:
					f_close( &f );
				case FR_NO_PATH:
				case FR_NO_FILE:
				{
					HardDriveConnected = 1;
					fres = FR_OK;
				} break;
				default:
				case FR_DISK_ERR:
				{
					dbgprintf("DIP: Disk error\n", fres );
					f_mount(0,0);
					udelay(500000);
				} break;
			}
		}

		if( fres != FR_OK )
		{
			DIP_Fatal("main()", __LINE__, __FILE__, 0, "Could not find any USB device!");
			ThreadCancel( 0, 0x77 );
		}

		FHandle = (FIL*)malloc( sizeof(FIL) * MAX_HANDLES );
		
		for( i=0; i < MAX_HANDLES; i++ )
			FHandle[i].fs = (FATFS*)NULL;

	} else {
		dbgprintf("DIP:Found FS-USB\n");
		FSMode = UNEEK;
	}

	FSHandle = IOS_Open("/dev/fs", 0 );
	if( FSHandle < 0 )
		dbgprintf("DIP:Error could not open /dev/fs! ret:%d\n", FSHandle );

	Nhandle = (s32*)malloc( sizeof(s32) * MAX_HANDLES );
	EntryCount = (u32*)malloc( sizeof(s32) );
		
	for( i=0; i < MAX_HANDLES; i++ )
		Nhandle[i] = 0xdeadbeef;

	Entries = (char*)NULL;

	return;
}
s32 DVDOpen( char *Filename, u32 Mode )
{
	int i,ret;
	if( FSMode == SNEEK )
	{
		u32 handle = 0xdeadbeef;

		//Find free handle
		for( i=0; i < MAX_HANDLES; i++ )
		{
			if( FHandle[i].fs == NULL )
			{
				handle = i;
				break;
			}
		}

		if( handle == 0xdeadbeef )
		{
			dbgprintf("DIP:Couldn't find a free handle!\n");			
			return FR_DISK_ERR;
		}
		
		ret = f_open( &(FHandle[handle]), Filename, Mode );
		if( ret != FR_OK )
		{
			FHandle[handle].fs = (FATFS*)NULL;
			switch( ret )
			{
				case FR_NO_PATH:
				case FR_NO_FILE:
					return DVD_NO_FILE;
					break;
				default:
					//dbgprintf("DIP:DVDOpen() ret:%d\n", ret );
					return DVD_FATAL;
					break;
			}
		}
		
		return handle;

	} else {
		
		u32 handle = 0xdeadbeef;

		//Find free handle
		for( i=0; i < MAX_HANDLES; i++ )
		{
			if( Nhandle[i] == 0xdeadbeef )
			{
				handle = i;
				break;
			}
		}

		if( handle == 0xdeadbeef )
		{
			dbgprintf("DIP:Couldn't find a free handle!\n");
			return DVD_FATAL;
		}
				
		char *name = (char*)halloca( 256, 32 );
		memcpy( name, Filename, strlen(Filename) + 1 );
		vector *vec = (vector*)halloca( sizeof(vector) * 2, 32 );
		
		vec[0].data = (u8*)name;
		vec[0].len = 256;
		vec[1].data = (u8*)Mode;
		vec[1].len = sizeof(u32);

		Nhandle[handle] = IOS_Ioctlv( FSHandle, 0x60, 2, 0, vec );
		
		name[0] = '/';
		name[1] = 'A';
		name[2] = 'X';
		name[3] = '\0';

		Nhandle[handle] = IOS_Open( name, Mode & ( FA_READ | FA_WRITE ) );

		hfree( name );
		hfree( vec );

		if( Nhandle[handle] < 0 )
		{
			handle = Nhandle[handle];
			Nhandle[handle]  =0xdeadbeef;
		}
		
		return handle;
	}
	

	return DVD_FATAL;
}

s32 DVDOpenDir( char *Path )
{
	if( FSMode == SNEEK )
	{
		return f_opendir( &d, Path );
	} else {
				
		char *name = (char*)halloca( 256, 32 );

		memcpy( name, Path, strlen(Path) + 1 );

		s32 ret = ISFS_ReadDir( name, (char*)NULL, EntryCount );
		if( ret < 0 )
		{
			dbgprintf("DIP:ISFS_ReadDir(\"%s\"):%d\n", name, ret );
			hfree(name);
			return ret;
		}

		if( Entries != NULL )
			hfree(Entries);

		Entries = (char*)halloca( *EntryCount * 64, 32 );

		ret = ISFS_ReadDir( name, Entries, EntryCount );
		if( ret < 0 )
		{
			dbgprintf("DIP:ISFS_ReadDir(\"%s\"):%d\n", name, ret );
		}

		CEntry = Entries;
		FirstEntry = 0;
		
		hfree(name);
		return ret;
	}
	return DVD_FATAL;
}
s32 DVDReadDir( void )
{
	if( FSMode == SNEEK )
	{
		if( f_readdir( &d, &FInfo ) == FR_OK )
		{
			return DVD_SUCCESS;
		} else 
			return DVD_NO_FILE;

	} else {

		if( FirstEntry > 0 )
		{
			//seek to next entry
			while( *CEntry++ != '\0' );
		}

		FirstEntry++;

		if( FirstEntry <= *EntryCount )
			return DVD_SUCCESS;
		else
			return DVD_NO_FILE;

	}
	return DVD_FATAL;
}
s32 DVDDirIsFile( void )
{
	if( FSMode == SNEEK )
	{
		if( (FInfo.fattrib & AM_DIR) == 0 )
			return 1;
		else
			return 0;
	} else {
		/*
			This is a hack since it would be very complicated to figure out if an entry is a file or a folder
		*/
		return 0;	
	}
	return DVD_FATAL;
}
char *DVDDirGetEntryName( void )
{
	if( FSMode == SNEEK )
	{
		if( FInfo.lfsize )
			return FInfo.lfname;
		else
			return FInfo.fname;
	} else {
		return CEntry;
	}

	return (char*)NULL;
}
u32 DVDRead( s32 handle, void *ptr, u32 len )
{
	u32 read;

	if( FSMode == SNEEK )
	{
		if( handle >= MAX_HANDLES )
			return 0;
		if( FHandle[handle].fs == NULL )
			return 0;

		f_read( &(FHandle[handle]), ptr, len, &read );

		return read;

	} else {

		if( handle >= MAX_HANDLES )
			return 0;
		if( Nhandle[handle] == 0xdeadbeef )
			return 0;

		return IOS_Read( Nhandle[handle], ptr, len );
	}

	return DVD_FATAL;
}
u32 DVDWrite( s32 handle, void *ptr, u32 len )
{
	u32 wrote;
	if( FSMode == SNEEK )
	{
		if( handle >= MAX_HANDLES )
			return 0;
		if( FHandle[handle].fs == NULL )
			return 0;

		f_write( &(FHandle[handle]), ptr, len, &wrote );

		return wrote;

	} else {
		
		if( handle >= MAX_HANDLES )
			return 0;
		if( Nhandle[handle] == 0xdeadbeef )
			return 0;

		return IOS_Write( Nhandle[handle], ptr, len );
	}

	return DVD_FATAL;
}
s32 DVDSeek( s32 handle, s32 where, u32 whence )
{
	if( FSMode == SNEEK )
	{
		if( handle >= MAX_HANDLES )
			return 0;
		if( FHandle[handle].fs == NULL )
			return 0;

		switch( whence )
		{
			case 0:
				if( f_lseek( &FHandle[handle], where ) == FR_OK )
					return where;
			break;

			case 1:
				if( f_lseek(&FHandle[handle], where + FHandle[handle].fptr) == FR_OK )
					return FHandle[handle].fptr;
			break;

			case 2:
				if( f_lseek(&FHandle[handle], where + FHandle[handle].fsize) == FR_OK )
					return FHandle[handle].fptr;
			break;

			default:
				break;
		}

	} else {
		
		if( handle >= MAX_HANDLES )
			return 0;
		if( Nhandle[handle] == 0xdeadbeef )
			return 0;

		return IOS_Seek( Nhandle[handle], where, whence );
	}

	return DVD_FATAL;
}
u32 DVDGetSize( s32 handle )
{
	if( FSMode == SNEEK )
	{
		if( handle >= MAX_HANDLES )
			return 0;
		if( FHandle[handle].fs == NULL )
			return 0;

		return FHandle[handle].fsize;

	} else {

		if( handle >= MAX_HANDLES )
			return 0;
		if( Nhandle[handle] == 0xdeadbeef )
			return 0;

		u32 size = 0;
		fstats *status = (fstats *)halloca( sizeof(fstats), 32 );

		if( ISFS_GetFileStats( Nhandle[handle], status ) >= 0 )
			size = status->Size;

		hfree( status );

		return size;		
	}

	return 0;
}
s32 DVDDelete( char *Path )
{
	if( FSMode == SNEEK )
	{
		switch( f_unlink( Path ) )
		{
			case FR_OK:
			case FR_NO_FILE:
			case FR_NO_PATH:
				return DVD_SUCCESS;
			break;
			default:
				return DVD_FATAL;
		}
	} else {
		return ISFS_Delete( Path );
	}
	return DVD_FATAL;
}
s32 DVDCreateDir( char *Path )
{
	if( FSMode == SNEEK )
	{
		if( f_mkdir( Path ) == FR_OK )
			return DVD_SUCCESS;
		else
			return DVD_FATAL;
	} else {
		return ISFS_CreateDir( Path, 0, 3, 3, 3);
	}
}
void DVDClose( s32 handle )
{
	if( FSMode == SNEEK )
	{
		if( handle >= MAX_HANDLES )
			return;
		if( FHandle[handle].fs == NULL )
			return;

		f_close( &(FHandle[handle]) );
		FHandle[handle].fs = (FATFS*)NULL;

	} else {

		if( handle >= MAX_HANDLES )
			return;
		if( Nhandle[handle] == 0xdeadbeef )
			return;

		IOS_Close( Nhandle[handle] );
		Nhandle[handle] = 0xdeadbeef;		
	}
	return;
}
