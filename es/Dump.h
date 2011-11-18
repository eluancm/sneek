#ifndef __DUMP__
#define __DUMP__

#include "global.h"
#include "DI.h"
#include "font.h"
#include "SMenu.h"

extern DIConfig *DICfg;
extern u32 *FB;

typedef struct
{
	u32 Unknown;
	u32 TMDSize;
	u32 TMDOffset;
	u32 CertSize;
	u32 CertOffset;
	u32 H3Offset;
	u32 DataOffset;
	u32 DataSize;

} PartOffsets;


typedef struct
{
	union
	{
		struct
		{
			u32 Type		:8;
			u32 NameOffset	:24;
		};
		u32 TypeName;
	};
	union
	{
		struct		// File Entry
		{
			u32 FileOffset;
			u32 FileLength;
		};
		struct		// Dir Entry
		{
			u32 ParentOffset;
			u32 NextOffset;
		};
		u32 entry[2];
	};
} FEntry;

u32 DumpDoTick( u32 CurrentFB );


#endif
