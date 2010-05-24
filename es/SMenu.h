
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

#ifndef _SMENU_
#define _SMENU_

#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "gecko.h"
#include "alloc.h"
#include "vsprintf.h"
#include "GCPad.h"
#include "WPad.h"
#include "DI.h"
#include "Font.h"


#define MENU_POS_X 20
#define MENU_POS_Y 80

s32 SMenuInit( u64 TitleID, u16 TitleVersion );
void SMenuAddFramebuffer( void );
void SMenuDraw( void );
void SMenuReadPad( void );

#endif
