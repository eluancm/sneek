#ifndef _CFG_
#define _CFG_

#include "string.h"
#include "global.h"

typedef struct DML_CFG 
{
	u32		Magicbytes;		// 0xD1050CF6
	u32		Version;		// 0x00000001
	u32		VideoMode;
	u32		Config;
	char	GamePath[255];
	char	CheatPath[255];
} DML_CFG;

enum dmlconfig
{
	DML_CFG_CHEATS		= (1<<0),
	DML_CFG_DEBUGGER	= (1<<1),
	DML_CFG_DEBUGWAIT	= (1<<2),
	DML_CFG_NMM			= (1<<3),
	DML_CFG_NMM_DEBUG	= (1<<4),
	DML_CFG_GAME_PATH	= (1<<5),
	DML_CFG_CHEAT_PATH	= (1<<6),
	DML_CFG_ACTIVITY_LED= (1<<7),
	DML_CFG_PADHOOK		= (1<<8),
	DML_CFG_FORCE_WIDE	= (1<<9),
	DML_CFG_BOOT_DISC	= (1<<10),
	DML_CFG_BOOT_DISC2	= (1<<11),
	DML_CFG_NODISC		= (1<<12),
};

enum dmlvideomode
{
	DML_VID_AUTO		= (0<<16),
	DML_VID_FORCE		= (1<<16),
	DML_VID_NONE		= (2<<16),

	DML_VID_MASK		= DML_VID_AUTO|DML_VID_FORCE|DML_VID_NONE,

	DML_VID_FORCE_PAL50	= (1<<0),
	DML_VID_FORCE_PAL60	= (1<<1),
	DML_VID_FORCE_NTSC	= (1<<2),
	DML_VID_FORCE_PROG	= (1<<3),
	
	DML_VID_FORCE_MASK	= DML_VID_FORCE_PAL50|DML_VID_FORCE_PAL60|DML_VID_FORCE_NTSC,

	DML_VID_PROG_PATCH	= (1<<4),	// Patches the game to use Prog mode no matter what
};

enum VideoModes
{
	GCVideoModeNone		= 0,
	GCVideoModePAL60	= 1,
	GCVideoModeNTSC		= 2,
	GCVideoModePROG		= 3,
};

#endif