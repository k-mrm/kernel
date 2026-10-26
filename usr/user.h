#ifndef _USER_H
#define _USER_H

#define SYS_NULL    0
#define SYS_EXEC    1
#define SYS_READ    2
#define SYS_WRITE   3
#define SYS_FORK    4
#define SYS_EXIT    5
#define SYS_WAIT    6

#ifndef __ASSEMBLER__

int exec(const char*, const char**);
int write(int, void*, int);
int read(int, void*, int);
int fork(void);
int exit(int);
int wait(int*);

#define NULL	((void *)0)

typedef _Bool bool;

#define true	1
#define false	0

#define va_list         __builtin_va_list
#define va_start(v, l)  __builtin_va_start (v, l)
#define va_arg(v, l)    __builtin_va_arg (v, l)
#define va_end(v)       __builtin_va_end (v)
#define va_copy(d, s)   __builtin_va_copy (d, s)

int printf(const char *fmt, ...);

#endif  // __ASSEMBLER__

#endif
