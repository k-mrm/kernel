#ifndef _MEMLAYOUT_H
#define _MEMLAYOUT_H

#include <kernel.h>
#include <sys.h>
#include <asm.h>

extern char __kstart[], __kend[];
extern char __ktext[], __ktext_e[];
extern char __rodata[], __rodata_e[];
extern char __kinit[], __kinit_e[];

struct e820_entry {
  u64 base, len;
  u32 type;
} PACKED; 
extern e820_entry e820[];
extern u16 e820count;

static inline ulong
pa(void *p)
{
  ulong va = (ulong)p;
  return va - PAGE_OFFSET;
}

static inline void *
va(ulong pa)
{
  return (void*)(pa + PAGE_OFFSET);
}

#define IS_KERN_TEXT(_va)     ((ulong)__ktext <= (ulong)(_va) && (ulong)(_va) < (ulong)__ktext_e)
#define IS_KERN_RODATA(_va)   ((ulong)__rodata <= (ulong)(_va) && (ulong)(_va) < (ulong)__rodata_e)
#define IS_KINIT(_va)         ((ulong)__kinit <= (ulong)(_va) && (ulong)(_va) < (ulong)__kinit_e)

#endif
