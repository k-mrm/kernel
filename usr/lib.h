#ifndef _ULIB_H
#define _ULIB_H

typedef unsigned long u64;
typedef signed long i64;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned char u8;
typedef signed char i8;

#define NULL ((void *)0)

typedef _Bool bool;

#define true 1
#define false 0

#define va_list __builtin_va_list
#define va_start  __builtin_va_start
#define va_arg  __builtin_va_arg
#define va_end __builtin_va_end

#endif
