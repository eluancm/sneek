#ifndef __COMMON_H__
#define __COMMON_H__

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0

extern void *memset8( void *dst, int x, size_t len );
extern void *memset16( void *dst, int x, size_t len );
extern void *memset32( void *dst, int x, size_t len );
extern void *memset8( void *dst, int x, size_t len );

void udelay(int us);

#define TICKET_SIZE	0x2A4

enum ContentType
{
	CONTENT_REQUIRED=	(1<<0),
	CONTENT_SHARED	=	(1<<15),
};

typedef struct
{
	u32 ID;				//	0
	u16 Index;			//	4
	u16 Type;			//	6
	u64 Size;			//	8
	u8	SHA1[20];		//  12
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

#endif
