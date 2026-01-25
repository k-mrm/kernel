#include <kernel.h>
#include "arch.h"
#include "seg.h"

ulong gdt[NR_SEG];

/*
const ulong gdt[] = {
        0x0,                // NULL
        0x00cf9b000000ffff, // KCODE32
        0x00cf93000000ffff, // KDATA32
        0x00af9b000000ffff, // KCODE64
        0x00af93000000ffff, // KDATA64
};
*/

TSS tss = {0};

typedef struct GDTDESC  GDTDESC;
typedef struct TSSDESC  TSSDESC;

struct GDTDESC {
        u16     limit_15_0;
        u16     base_15_0;
        u8      base_23_16;
        u8      type : 4;
        u8      s : 1;
        u8      dpl : 2;
        u8      p : 1;
        u8      lim_19_16 : 4;
        u8      avl : 1;
        u8      l : 1;
        u8      db : 1;
        u8      g : 1;
        u8      base_31_24;
} PACKED;

struct TSSDESC {
        u16     limit_15_0;
        u16     base_15_0;
        u8      base_23_16;
        u8      type : 4;
        u8      s : 1;
        u8      dpl : 2;
        u8      p : 1;
        u8      lim_19_16 : 4;
        u8      avl : 1;
        u8      l : 1;
        u8      db : 1;
        u8      g : 1;
        u8      base_31_24;
        u32     base_63_32;
        u32     reserved;
} PACKED;

static void
setseg (ulong *gdt, int type, u32 base, u32 limit, int dpl, int l)
{
        GDTDESC *desc = (GDTDESC *)gdt;

        desc->limit_15_0 = (limit >> 12) & 0xffff;
        desc->base_15_0 = base & 0xffff;
        desc->base_23_16 = (base >> 16) & 0xff;
        desc->type = type;
        desc->s = 1;
        desc->dpl = dpl;
        desc->p = 1;
        desc->lim_19_16 = (limit >> 28) & 0xf;
        desc->avl = 0;
        desc->l = l;
        desc->db = 0;
        desc->g = 1;
        desc->base_31_24 = (base >> 24) & 0xff;
}

static void
settssseg (ulong *gdt, int type, u64 base, u32 limit, int dpl)
{
        TSSDESC *desc = (TSSDESC *)gdt;

        desc->limit_15_0 = limit & 0xffff;
        desc->base_15_0 = base & 0xffff;
        desc->base_23_16 = (base >> 16) & 0xff;
        desc->type = type;
        desc->s = 0;
        desc->dpl = dpl;
        desc->p = 1;
        desc->lim_19_16 = (limit >> 16) & 0xf;
        desc->avl = 0;
        desc->l = 0;
        desc->db = 0;
        desc->g = 0;
        desc->base_31_24 = (base >> 24) & 0xff;
        desc->base_63_32 = base >> 32;
        desc->reserved = 0;
}

static void
tssinit (void)
{
        settssseg (gdt + SEG_TSS, STA_TSSA, (u64)&tss, sizeof(tss) - 1, DPL_KERNEL);
}

static void
configgdt (void)
{
        gdt[SEG_NULL] = 0;     // NULL Descriptor
        setseg (gdt + SEG_KCODE32, STA_X|STA_R, 0, 0xffffffff, DPL_KERNEL, 0);
        setseg (gdt + SEG_KDATA32, STA_W, 0, 0xffffffff, DPL_KERNEL, 0);
        setseg (gdt + SEG_KCODE, STA_X|STA_R, 0, 0xffffffff, DPL_KERNEL, 1);
        setseg (gdt + SEG_KDATA, STA_W, 0, 0xffffffff, DPL_KERNEL, 1);
        setseg (gdt + SEG_UCODE, STA_X|STA_R, 0, 0xffffffff, DPL_USER, 1);
        setseg (gdt + SEG_UDATA, STA_W, 0, 0xffffffff, DPL_USER, 1);
        tssinit ();
}

static inline void
loadgdt (const ulong *gdt, ulong gdtsize)
{
        volatile u16 t[5];

        t[0] = (u16)gdtsize - 1;
        t[1] = (u16)(ulong)gdt;
        t[2] = (u16)((ulong)gdt >> 16);
        t[3] = (u16)((ulong)gdt >> 32);
        t[4] = (u16)((ulong)gdt >> 48);

        asm volatile ("lgdt (%0)" :: "r"(t));
}

void
gdtinit (void)
{
        configgdt ();
        loadgdt (gdt, sizeof gdt);
        ltr (SEG_TSS << 3);
}
