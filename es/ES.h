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
#define ES_NFOUND		-106
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

#define TICKET_SIZE		0x2A4

enum ContentType
{
	CONTENT_REQUIRED=	(1<<0),
	CONTENT_SHARED	=	(1<<15),
};

typedef struct
{
	u32 ID;				//	0	(0x1E4)
	u16 Index;			//	4	(0x1E8)
	u16 Type;			//	6	(0x1EA)
	u64 Size;			//	8	(0x1EC)
	u8	SHA1[20];		//  12	(0x1F4)
} __attribute__((packed)) Content;

typedef struct
{
	u32 SignatureType;		// 0x000
	u8	Signature[0x100];	// 0x004

	u8	Padding0[0x3C];		// 0x104
	u8	Issuer[0x40];		// 0x140

	u8	Version;			// 0x180
	u8	CACRLVersion;		// 0x181
	u8	SignerCRLVersion;	// 0x182
	u8	Padding1;			// 0x183

	u64	SystemVersion;		// 0x184
	u64	TitleID;			// 0x18C 
	u32	TitleType;			// 0x194 
	u16	GroupID;			// 0x198 
	u8	Reserved[62];		// 0x19A 
	u32	AccessRights;		// 0x1D8
	u16	TitleVersion;		// 0x1DC 
	u16	ContentCount;		// 0x1DE 
	u16 BootIndex;			// 0x1E0
	u8	Padding3[2];		// 0x1E2 

	Content Contents[];		// 0x1E4 

} __attribute__((packed)) TitleMetaData;

typedef struct
{
	u32	SignatureType;			// 0x000
	u8	Signature[0x100];		// 0x004

	u8	Padding[0x3C];			// 0x104
	s8	SignatureIssuer[0x40];	// 0x140
	u8	DownloadContent[0x3F];	// 0x180
	u8	EncryptedTitleKey[0x10];// 0x1BF
	u8	Unknown;				// 0x1CF
	u64	TicketID;				// 0x1D0
	u32	ConsoleID;				// 0x1D8
	u64	TitleID;				// 0x1DC
	u16 UnknownA;				// 0x1E4
	u16 BoughtContents;			// 0x1E6
	u8	UknownB[0x08];			// 0x1E9
	u8	CommonKeyIndex;			// 0x1F1
	u8	UnknownC[0x30];			// 0x1F2
	u8	UnknownD[0x20];			// 0x222
	u16 PaddingA;				// 0x242
	u32 TimeLimitEnabled;		// 0x248
	u32 TimeLimit;				// 0x24C

} __attribute__((packed)) Ticket;


typedef struct
{
	u8	Unknown;				// 0x000
	u8	Padding[3];				// 0x001
	u64	TicketID;				// 0x004
	u32	ConsoleID;				// 0x00C
	u64	TitleID;				// 0x010
	u16 UnknownA;				// 0x
	u16 BoughtContents;			// 0x
	u8	UknownB[0x08];			// 0x
	u8	CommonKeyIndex;			// 0x
	u8	UnknownC[0x30];			// 0x
	u8	UnknownD[0x20];			// 0x
	u16 PaddingA;				// 0x
	u32 TimeLimitEnabled;		// 0x
	u32 TimeLimit;				// 0x

} __attribute__((packed)) TicketView;

typedef struct
{
	u64	TitleID;				//
	u16 Padding;				//
	u16 GroupID;				//

} __attribute__((packed)) UIDSYS;

u32 ES_Init( u8 *MessageHeap );

void ES_Ioctlv( struct ipcmessage *msg );

s32 ES_LoadModules( u32 KernelVersion );
s32 ES_LaunchSYS( u64 *TitleID );

u8 *NANDLoadFile( char * path, u32 *Size );
s32 doTicketMagic( Ticket *iTIK );

u64 ES_GetTitleID( void );

s32 ES_CreateKey( u8 *Ticket );
s32 ES_CheckBootOption( char *Path, u64 *TitleID );
s32 ES_LaunchTitle( u64 *TitleID, u8 *TikView );
s32 ESP_Sign( u64 *TitleID, u8 *data, u32 len, u8 *sign, u8 *cert );
s32 ES_BootSystem( u64 *TitleID, u32 *KernelVersion );
s32 ES_CheckSharedContent( void *ContentHash );
s32 ES_GetS1ContentID( void *ContentHash );

#endif
