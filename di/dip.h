#ifndef _DIP
#define _DIP

#include "string.h"
#include "syscalls.h"
#include "global.h"
#include "ipc.h"
#include "ehci_types.h"
#include "ehci.h"
#include "gecko.h"
#include "alloc.h"

#define	DI_BASE			0x0D806000

#define DI_STATUS		*(vu32*)(DI_BASE)
#define DI_COVER		*(vu32*)(DI_BASE+0x04)

#define DI_CMD0			*(vu32*)(DI_BASE+0x08)
#define DI_CMD1			*(vu32*)(DI_BASE+0x0C)
#define DI_CMD2			*(vu32*)(DI_BASE+0x10)

#define DI_DMA_ADR		*(vu32*)(DI_BASE+0x14)
#define DI_DMA_LEN		*(vu32*)(DI_BASE+0x18)

#define DI_CR			*(vu32*)(DI_BASE+0x1C)
#define DI_IMM			*(vu32*)(DI_BASE+0x20)

#define DMA_READ		3
#define IMM_READ		1

s32 InitRegisters( void );
s32 DIReadDiscID( void );

enum opcodes
{
	DVD_IDENTIFY			= 0x12,
	DVD_READ_DISCID			= 0x70,
	DVD_LOW_READ			= 0x71,
	DVD_WAITFORCOVERCLOSE	= 0x79,
	DVD_READ_PHYSICAL		= 0x80,
	DVD_READ_COPYRIGHT		= 0x81,
	DVD_READ_DISCKEY		= 0x82,
	DVD_GETCOVER			= 0x88,
	DVD_RESET				= 0x8A,
	DVD_OPEN_PARTITION		= 0x8B,
	DVD_CLOSE_PARTITION		= 0x8C,
	DVD_READ_UNENCRYPTED	= 0x8D,
	DVD_REPORTKEY			= 0xA4,
	DVD_LOW_SEEK			= 0xAB,
	DVD_READ				= 0xD0,
	DVD_READ_CONFIG			= 0xD1,
	DVD_READ_BCA			= 0xDA,
	DVD_GET_ERROR			= 0xE0,
	DVD_SET_MOTOR			= 0xE3,

	DVD_SELECT_GAME			= 0x23,
	DVD_GET_GAMECOUNT		= 0x24,
	DVD_EJECT_DISC			= 0x27,
	DVD_INSERT_DISC			= 0x28,
	DVD_READ_GAMEINFO		= 0x30,
};

#define DI_SUCCESS	1
#define DI_ERROR	2
#define DI_FATAL	64

#endif