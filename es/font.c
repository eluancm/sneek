/*

preloader 0.30 - A tool which allows to change the default boot up sequence on the Wii console

Copyright (C) 2008-2009  crediar

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
#include "font.h"
#include "DI.h"

u32 *fontfx = (u32*)NULL;

void PrintChar( u32 FrameBuffer, int xx, int yy, char c )
{
	u32* fb = (u32*)FrameBuffer;

	if( fb == NULL )
		return;

	if( c >= 0x7F || c < 0x20 )
		c = ' ';

	int x,y;
	for( x=1; x <7; ++x)
	{
		for( y=0; y<16; ++y)
		{
			fb[(x+xx)+(y+yy)*320] = fontfx[x+(y+(c-' ')*16)*8];
		}
	}
}

void PrintString( u32 FrameBuffer, int x, int y, char *str )
{
	int i=0;
	while(str[i]!='\0')
	{
		PrintChar( FrameBuffer, x+i*6, y, str[i] );
		i++;
	}
}
void PrintFormat( u32 FrameBuffer, int x, int y, const char *str, ... )
{
	if( fontfx == NULL )
		return;

	char *astr = (char*)malloc( 2048 );
	memset32( astr, 0, 2048 );

	va_list ap;
	va_start( ap, str );

	vsprintf( astr, str, ap);

	va_end( ap );

	PrintString( FrameBuffer, x, y, astr );

	free(astr);
}
s32 LoadFont( char *str )
{
	u32 *size = (u32*)malloca( sizeof(u32), 32 );

	fontfx = (u32*)NANDLoadFile( str, size );
	if( fontfx == NULL )
	{
		dbgprintf("ES:Couldn't open:\"%s\":%d\n", str, *size );
		free( size );
		//Try USB device

		s32 fd = DVDOpen( str );
		if( fd < 0 )
		{
			return 0;

		} else {

			fontfx = (u32*)heap_alloc_aligned( 0, 0xBE00, 32 );
			DVDRead( fd, fontfx, 0xBE00 );
			DVDClose( fd );
			
			return 1;
		}
	}

	free( size );
	return 1;
}