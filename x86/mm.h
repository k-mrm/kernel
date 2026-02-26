#ifndef _ARCH_MM_H
#define _ARCH_MM_H

#include "arch.h"
#include "seg.h"

#define PTE_P (1 << 0)
#define PTE_W (1 << 1)
#define PTE_U (1 << 2)
#define PTE_PWT (1 << 3)
#define PTE_PCD (1 << 4)
#define PTE_A (1 << 5)
#define PTE_D (1 << 6)
#define PTE_G (1 << 8)
#define PTE_XD (1ull << 63)

/*
 *  x86 48bit Virtual Address
 *
 *     48    39 38    30 29    21 20    12 11       0
 *    +--------+--------+--------+--------+----------+
 *    | level4 | level3 | level2 | level1 | page off |
 *    +--------+--------+--------+--------+----------+
 *       pml4     pdpt      pd       pt
 *
 */

#define PIDX(_level, _addr) (((_addr) >> (12 + ((_level) - 1) * 9)) & 0x1ff)

#define PAGESIZE 0x1000
#define PAGESHIFT 12

#define PTE_PA_MASK ULL(0xfffffffff000)

// Direct mapping offset: 0xffff800000000000 - 0xffffc00000000000

#define KLINK_OFFSET ULL(0xffff800000000000)
#define KERNLINK ULL(0xffff800000100000)
#define KERNLINK_PA ULL(0x100000)

#define PAGE_OFFSET KLINK_OFFSET

#ifndef __ASSEMBLER__

#define PTE_PA(_pte) ((ulong)(_pte) & PTE_PA_MASK)
#define PTE_FLAGS(_pte) ((ulong)(_pte) & ~(PTE_PA_MASK))

static inline PTE nexttablepte(Phys ntaddr) {
  return (ntaddr & PTE_PA_MASK) | PTE_P | PTE_W | PTE_U;
}

static inline ulong ppresent(void) { return PTE_P; }

static inline ulong paccess(void) { return 0; }

static inline ulong preadonly(void) { return 0; }

static inline ulong pwritable(void) { return PTE_W; }

static inline ulong puser(void) { return PTE_U; }

static inline ulong pexecutable(void) { return 0; }

static inline ulong pnox(void) { return PTE_XD; }

static inline ulong pnocache(void) { return PTE_PCD; }

static inline ulong pnormal(void) { return 0; }

static inline ulong pdevice(void) { return 0; }

static inline PTE pteleaf(Phys pa, ulong archflags) {
  return (pa & PTE_PA_MASK) | archflags | PTE_P;
}

extern char __kstart[], __kend[];
extern char __ktext[], __ktext_e[];
extern char __rodata[], __rodata_e[];
extern char __kinit[], __kinit_e[];

static inline Phys V2P(void *p) {
  ulong va = (ulong)p;
  return va - PAGE_OFFSET;
}

static inline void *P2V(Phys pa) { return (void *)(pa + PAGE_OFFSET); }

#define IS_KERN_TEXT(_va)                                                      \
  ((ulong)__ktext <= (ulong)(_va) && (ulong)(_va) < (ulong)__ktext_e)
#define IS_KERN_RODATA(_va)                                                    \
  ((ulong)__rodata <= (ulong)(_va) && (ulong)(_va) < (ulong)__rodata_e)
#define IS_KINIT(_va)                                                          \
  ((ulong)__kinit <= (ulong)(_va) && (ulong)(_va) < (ulong)__kinit_e)

typedef struct Vas Vas;

void switchvas(Vas *vas);
void killbootmap(void);
void x86mminit(void);

#endif // __ASSEMBLER__
#endif // _ARCH_MM_H
