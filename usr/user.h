#ifndef _USER_H
#define _USER_H

#define SYS_NULL    0
#define SYS_EXEC    1
#define SYS_READ    2
#define SYS_WRITE   3
#define SYS_FORK    4
#define SYS_EXIT    5
#define SYS_WAIT    6
#define SYS_OPEN    7
#define SYS_CLOSE   8

#ifndef __ASSEMBLER__

int exec(const char*, const char**);
int read(int, void*, int);
int write(int, void*, int);
int fork(void);
int exit(int);
int wait(int*);
int open(char*, int);
int close(int);

#define NULL	((void *)0)

typedef _Bool bool;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned char uchar;

#define true	1
#define false	0

#define va_list         __builtin_va_list
#define va_start(v, l)  __builtin_va_start (v, l)
#define va_arg(v, l)    __builtin_va_arg (v, l)
#define va_end(v)       __builtin_va_end (v)
#define va_copy(d, s)   __builtin_va_copy (d, s)

int printf(const char *fmt, ...);
void *memcpy(void*, const void*, ulong);
void *memmove(void*, const void*, ulong);
void *memset(void*, int, ulong);
char *strcpy(char*, const char*);
int strcmp(const char*, const char*);
int memcmp(const void*, const void*, uint);
int strncmp(const char*, const char*, ulong);
uint strlen(const char*);
char *strchr(const char*, int);
char *strtok(char*, const char*);

#define O_READ    0
#define O_WRITE   1
#define O_RDWR    2
#define O_EXEC    3
#define O_CREATE  0x10

#endif  // __ASSEMBLER__

#endif
