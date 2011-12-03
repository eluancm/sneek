#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define UINT_MAX ((unsigned int)0xffffffff)
#define MEM2_BSS __attribute__ ((section (".bss.mem2")))


#define DEBUG		0
#define false		0
#define true		1

#define	SHARED_PTR	((void *)0x13600000)
#define	SHARED_SIZE	(0x18000)

void fatal(const char *format, ...);

void udelay(int us);

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef int bool;
typedef unsigned int sec_t;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef volatile unsigned long long vu64;

typedef volatile signed char vs8;
typedef volatile signed short vs16;
typedef volatile signed int vs32;
typedef volatile signed long long vs64;

typedef s32 size_t;

typedef u32 u_int32_t;

typedef s32(*ipccallback)(s32 result,void *usrdata);

#define NULL ((void *)0)

#define ALIGNED(x) __attribute__((aligned(x)))

/* Attributes */
#ifndef ATTRIBUTE_ALIGN
# define ATTRIBUTE_ALIGN(v) __attribute__((aligned(v)))
#endif

#ifndef ATTRIBUTE_PACKED
# define ATTRIBUTE_PACKED __attribute__((packed))
#endif
typedef struct
{
	u32 data;
	u32 len;
} vector;

#define STACK_ALIGN(type, name, cnt, alignment)         \
	u8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + \
	(((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - \
	((sizeof(type)*(cnt))%(alignment))) : 0))]; \
	type *name = (type*)(((u32)(_al__##name)) + ((alignment) - (( \
	(u32)(_al__##name))&((alignment)-1))))

#endif

