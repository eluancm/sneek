#include <stdarg.h>
#include "syscalls.h"
#include "gecko.h"
#include "string.h"

int vsprintf(char *buf, const char *fmt, va_list args);
int _sprintf( char *buf, const char *fmt, ... );
int dbgprintf( u32 dbglevel, const char *fmt, ...);
void hexdump(void *d, int len);
