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

#endif
