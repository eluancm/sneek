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
#ifndef __ESP__
#define __ESP__

#include "global.h"
#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "common.h"
#include "FS.h"
#include "NAND.h"
#include "alloc.h"
#include "DI.h"

void ES_Fatal( char *name, u32 line, char *file, s32 error, char *msg );

void iCleanUpTikTMD( void );

s32 ES_TitleCreatePath( u64 TitleID );

void iES_GetTMDView( TitleMetaData *TMD, u8 *oTMDView );
void iES_GetTicketView( u8 *Ticket, u8 *oTicketView );

s32 ESP_GetTMDView( u64 *TitleID, u8 *oTMDView );
s32 ESP_DIGetTicketView( u64 *TitleID, u8 *oTikView );

s32 ESP_DIVerify( u64 *TitleID, u32 *Key, TitleMetaData *TMD, u32 tmd_size, char *tik, char *Hashes );
s32 ESP_GetUID( u64 *TitleID, u16 *UID );

s32 ESP_OpenContent( u64 TitleID, u32 ContentID );

s32 ESP_AddContentFinish( u32 cid, u8 *Ticket, TitleMetaData *TMD );
s32 ESP_AddContentData(s32 cfd, void *data, u32 data_size);
s32 ESP_AddTitleFinish( TitleMetaData *TMD );

s32 ESP_GetTitles( u64 *Titles );
s32 ESP_GetNumTitles( u32 *TitleCount );

s32 ESP_GetOwnedTitles( u64 *Titles );
s32 ESP_GetNumOwnedTitles( u32 *TitleCount );

#endif
