/*

SNEEK - SD-NAND/ES emulation kit for Nintendo Wii

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
#ifndef __ES_STRUCT_H__
#define __ES_STRUCT_H__

#include "global.h"
#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "common.h"
#include "FS.h"
#include "NAND.h"
#include "alloc.h"

#define SHA_INIT 0
#define SHA_UPDATE 1
#define SHA_FINISH 2

#define ES_FD			154
#define SD_FD			155

// error codes
#define ES_SUCCESS		0
#define ES_FATAL		-1017
#define	ES_ETIKTMD		-1029
#define	ES_EHASH		-1022
#define ES_EFAIL		-4100

// ES ioctl's
#define IOCTL_ES_ADDTICKET				0x01
#define IOCTL_ES_ADDTITLESTART			0x02
#define IOCTL_ES_ADDCONTENTSTART		0x03
#define IOCTL_ES_ADDCONTENTDATA			0x04
#define IOCTL_ES_ADDCONTENTFINISH		0x05
#define IOCTL_ES_ADDTITLEFINISH			0x06
#define IOCTL_ES_GETDEVICEID			0x07
#define IOCTL_ES_LAUNCH					0x08
#define IOCTL_ES_OPENCONTENT			0x09
#define IOCTL_ES_READCONTENT			0x0A
#define IOCTL_ES_CLOSECONTENT			0x0B
#define IOCTL_ES_GETOWNEDTITLECNT		0x0C
#define IOCTL_ES_GETOWNEDTITLES			0x0D
#define IOCTL_ES_GETTITLECNT			0x0E
#define IOCTL_ES_GETTITLES				0x0F
#define IOCTL_ES_GETTITLECONTENTSCNT	0x10
#define IOCTL_ES_GETTITLECONTENTS		0x11
#define IOCTL_ES_GETVIEWCNT				0x12
#define IOCTL_ES_GETVIEWS				0x13
#define IOCTL_ES_GETTMDVIEWSIZE			0x14
#define IOCTL_ES_GETTMDVIEWS			0x15
#define IOCTL_ES_GETCONSUMPTION			0x16
#define IOCTL_ES_DELETETITLE			0x17
#define IOCTL_ES_DELETETICKET			0x18
#define IOCTL_ES_DIGETTMDVIEWSIZE		0x19
#define IOCTL_ES_DIGETTMDVIEW			0x1A
#define IOCTL_ES_DIGETTICKETVIEW		0x1B
#define IOCTL_ES_DIVERIFY				0x1C
#define IOCTL_ES_GETTITLEDIR			0x1D
#define IOCTL_ES_GETDEVICECERT			0x1E
#define IOCTL_ES_IMPORTBOOT				0x1F
#define IOCTL_ES_GETTITLEID				0x20
#define IOCTL_ES_SETUID					0x21
#define IOCTL_ES_DELETETITLECONTENT		0x22
#define IOCTL_ES_SEEKCONTENT			0x23
#define IOCTL_ES_OPENTITLECONTENT		0x24
#define IOCTL_ES_LAUNCHBC				0x25
#define IOCTL_ES_EXPORTTITLEINIT		0x26
#define IOCTL_ES_EXPORTCONTENTBEGIN		0x27
#define IOCTL_ES_EXPORTCONTENTDATA		0x28
#define IOCTL_ES_EXPORTCONTENTEND		0x29
#define IOCTL_ES_EXPORTTITLEDONE		0x2A
#define IOCTL_ES_ADDTMD					0x2B
#define IOCTL_ES_ENCRYPT				0x2C
#define IOCTL_ES_DECRYPT				0x2D
#define IOCTL_ES_GETBOOT2VERSION		0x2E
#define IOCTL_ES_ADDTITLECANCEL			0x2F
#define IOCTL_ES_SIGN					0x30
#define IOCTL_ES_VERIFYSIGN				0x31
#define IOCTL_ES_GETTMDCONTENTCNT		0x32
#define IOCTL_ES_GETTMDCONTENTS			0x33
#define IOCTL_ES_GETSTOREDTMDSIZE		0x34
#define IOCTL_ES_GETSTOREDTMD			0x35
#define IOCTL_ES_GETSHAREDCONTENTCNT	0x36
#define IOCTL_ES_GETSHAREDCONTENTS		0x37

#define IOCTL_ES_DIGETSTOREDTMDSIZE		0x39
#define IOCTL_ES_DIGETSTOREDTMD			0x3A

typedef struct
{
	u32 SignatureType;
	u8	Signature[0x100];
	u8	Padding[0x3C];
	s8	SignatureIssuer[0x40];
	u8	DownloadContent[0x3F];
	u8	EncryptedTitleKey[0x10];
	u8	Unknown;
	u8	TicketID[0x08];
	u32	ConsoleID;
	u8	TitleID[0x08];
	u16 UnknownA;
	u16 BoughtContents;
	u8	UknownB[0x08];
	u8 CommonKeyIndex;
	u8 UnknownC[0x30];
	u8 UnknownD[0x20];
	u16 PaddingA;
	u32 TimeLimitEnabled;
	u32 TimeLimit;

} TileMetaData;

s32 ES_LoadModules( u32 KernelVersion );
s32 ES_LaunchSYS( u64 *TitleID );
s32 ES_AddContentFinish( u32 cid, u8 *Ticket, u8 *TMD );
s32 ES_AddContentData(s32 cfd, void *data, u32 data_size);
s32 ES_AddTitleFinish( void *TMD );
u8 *NANDLoadFile( char * path, u32 *Size );
s32 doTicketMagic( u8 *iTIK );
s32 ES_CreateKey( u8 *Ticket );
s32 ES_DIVerify( u64 *TitleID, u32 *Key, void *tmd, u32 tmd_size, void *tik, void *Hashes );
s32 ES_CheckBootOption( char *Path, u64 *TitleID );
s32 ES_LaunchTitle( u64 *TitleID, u8 *TikView );
s32 ES_GetUID( u64 *TitleID, u16 *UID );
void iES_GetTMDView( u8 *TMD, u8 *oTMDView );
s32 ES_GetTMDView( u64 *TitleID, u8 *oTMDView );
s32 ES_GetNumOwnedTitles( u32 *TitleCount );
s32 ES_GetNumTitles( u32 *TitleCount );
s32 ES_Sign( u64 *TitleID, u8 *data, u32 len, u8 *sign, u8 *cert );
s32 ES_BootSystem( u64 *TitleID, u32 *KernelVersion );
s32 ES_DIGetTicketView( u64 *TitleID, u8 *oTikView );
s32 ES_GetOwnedTitles( u64 *Titles );
s32 ES_GetTitles( u64 *Titles );
s32 ES_CheckSharedContent( void *ContentHash );
s32 ES_GetS1ContentID( void *ContentHash );
void iES_GetTicketView( u8 *Ticket, u8 *oTicketView );
s32 ES_OpenContent( u64 TitleID, u32 ContentID );

#endif
