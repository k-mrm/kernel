#ifndef __X86_SEG_H
#define __X86_SEG_H

#define DPL_KERNEL      0x0
#define DPL_USER        0x3

#define STA_X       0x8
#define STA_W       0x2
#define STA_R       0x2

#define STA_TSSA    0x9

#define SEG_NULL        0x0
#define SEG_KCODE32     0x1
#define SEG_KDATA32     0x2
#define SEG_KCODE       0x3
#define SEG_KDATA       0x4
#define SEG_UCODE       0x5
#define SEG_UDATA       0x6
#define SEG_TSS         0x7
#define NR_SEG          9

#ifndef __ASSEMBLER__

#include <kernel.h>

typedef struct TSS      TSS;

struct TSS {
        u32     reserved0;
        u64     rsp0;
        u64     rsp1;
        u64     rsp2;
        u64     reserved1;
        u64     ist1;
        u64     ist2;
        u64     ist3;
        u64     ist4;
        u64     ist5;
        u64     ist6;
        u64     ist7;
        u64     reserved2;
        u16     reserved3;
        u16     io_map_base;
} PACKED;

extern TSS tss;

#endif  // __ASSEMBLER__
#endif  // __X86_SEG_H
