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
#define SYS_PIPE    9
#define SYS_DUP2    10

#ifndef __ASSEMBLER__

int exec(const char*, const char**);
int read(int, void*, int);
int write(int, void*, int);
int fork(void);
int exit(int);
int wait(int*);
int open(char*, int);
int close(int);
int pipe(int*);
int dup2(int, int);

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

#define S_IFMT    0xf000

#define S_IFSOCK  0xc000
#define S_IFLNK   0xa000
#define S_IFREG   0x8000
#define S_IFBLK   0x6000
#define S_IFDIR   0x4000
#define S_IFCHR   0x2000
#define S_IFIFO   0x1000

#define S_ISUID   0x0800
#define S_ISGID   0x0400
#define S_ISVTX   0x0200

#define S_IRUSR   0x0100
#define S_IWUSR   0x0080
#define S_IXUSR   0x0040
#define S_IRGRP   0x0020
#define S_IWGRP   0x0010
#define S_IXGRP   0x0008
#define S_IROTH   0x0004
#define S_IWOTH   0x0002
#define S_IXOTH   0x0001

#define S_ISSOCK(_m)    (((_m) & S_IFMT) == S_IFSOCK)
#define S_ISLNK(_m)     (((_m) & S_IFMT) == S_IFLNK)
#define S_ISREG(_m)     (((_m) & S_IFMT) == S_IFREG)
#define S_ISBLK(_m)     (((_m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(_m)     (((_m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(_m)     (((_m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(_m)    (((_m) & S_IFMT) == S_IFIFO)

#define O_READ    0
#define O_WRITE   1
#define O_RDWR    2
#define O_EXEC    3
#define O_CREATE  0x10
#define O_APPEND  0x20

#endif  // __ASSEMBLER__

#endif
