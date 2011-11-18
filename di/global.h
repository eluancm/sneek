#ifndef __GLOBAL_H__
#define __GLOBAL_H__

enum Debuglevel
{
	DEBUG_NONE = 0,
	DEBUG_NOTICE,
	DEBUG_ERROR,
	DEBUG_WARNING,
	DEBUG_INFO,
	DEBUG_DEBUG,
	DEBUG_ALL,
};

#define DEBUG
#define FILEMODE

#define	SHARED_PTR	((void *)0x13600000)
#define	SHARED_SIZE	(0x18000)


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

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

#define NULL ((void *)0)

#define ALIGNED(x) __attribute__((aligned(x)))

#define STACK_ALIGN(type, name, cnt, alignment)         \
	u8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + \
	(((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - \
	((sizeof(type)*(cnt))%(alignment))) : 0))]; \
	type *name = (type*)(((u32)(_al__##name)) + ((alignment) - (( \
	(u32)(_al__##name))&((alignment)-1))))


int dbgprintf( u32 dbglevel, const char *fmt, ...);

#define debug_printf dbgprintf

void fatal(const char *format, ...);

void udelay(int us);
void *memset32( void *dst, int x, size_t len );


#endif
