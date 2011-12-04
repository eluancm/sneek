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


u32 DumpDoTick( u32 CurrentFB );


#endif
