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

#endif
