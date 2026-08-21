#ifndef GQ_STRING_H
#define GQ_STRING_H

#include <stddef.h>

void *memset(void *s, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
int   memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
char *strchr(const char *s, int c);

#endif
