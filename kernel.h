#ifndef _KERNEL_H
#define _KERNEL_H

typedef unsigned long u64;
typedef long i64;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned char u8;
typedef signed char i8;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;

#define NULL	((void *)0)

typedef _Bool bool;

#define true	1
#define false	0

#define UNUSED          __attribute__((unused))
#define FALLTHROUGH     __attribute__((fallthrough))
#define PACKED          __attribute__((packed))
#define ALIGNED(n)      __attribute__((aligned(n)))
#define NORETURN        __attribute__((noreturn))
#define SECTION(s)      __attribute__((section(s)))
#define USED            __attribute__((used))

#define offsetof(st, m) ((u64)((char *)&((st *)0)->m - (char *)0))

#define container_of(ptr, st, m)  \
  ({ const typeof(((st *)0)->m) *_mptr = (ptr); \
   (st *)((char *)_mptr - offsetof(st, m)); })

#define MAX(_a, _b) ((_a) < (_b) ? (_b) : (_a))
#define MIN(_a, _b) ((_a) > (_b) ? (_b) : (_a))

#define va_list         __builtin_va_list
#define va_start(v, l)  __builtin_va_start (v, l)
#define va_arg(v, l)    __builtin_va_arg (v, l)
#define va_end(v)       __builtin_va_end (v)
#define va_copy(d, s)   __builtin_va_copy (d, s)

#endif
