#ifndef _STRING_H
#define _STRING_H

#include <kernel.h>

void *memcpy (void *dst, const void *src, ulong n);
void *memmove (void *dst, const void *src, ulong n);
void *memset (void *dst, int c, ulong n);
int strcmp (const char *s1, const char *s2);
int strncmp (const char *s1, const char *s2, ulong len);
uint strlen (const char *s);
char *strcpy (char *dst, const char *src);
char *strchr (const char *s, int c);
char *strtok (char *s1, const char *s2);

#endif  // _STRING_H
