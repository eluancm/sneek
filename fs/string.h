#include "global.h"

char *strcpy(char *, const char *);
char *strncpy(char *, const char *, size_t);
size_t strlcpy(char *dest, const char *src, size_t maxlen);
int strcmp(const char *, const char *);
int strncmp(const char *p, const char *q, size_t n);
size_t strlen(const char *);
size_t strnlen(const char *, size_t);
char *strchr(const char *s, int c);
int memcmp(const void *s1, const void *s2, size_t n);
int sprintf(char *str, const char *fmt, ...);
char * strstr (   const char * str1,  const char * str2   );
void hexdump(void *d, int len);
