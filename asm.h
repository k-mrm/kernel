#ifndef ASM_H
#define ASM_H

#ifdef __ASSEMBLER__
#define UL(_a)    _a
#define ULL(_a)   _a
#else // !__ASSEMBLER__
#define UL(_a)    _a##ul
#define ULL(_a)   _a##ull
#endif

#define CR0_PE		0x1
#define CR0_PG		0x80000000
#define CR4_VME       	0x1
#define CR4_PVI       	0x2
#define CR4_TSD       	0x4
#define CR4_DE        	0x8
#define CR4_PSE       	0x10
#define CR4_PAE       	0x20
#define CR4_MCE       	0x40
#define CR4_PGE       	0x80
#define CR4_PCE       	0x100
#define CR4_OSFXSR    	0x200
#define CR4_OSXMMEXCPT	0x400
#define CR4_VMXE      	0x2000
#define CR4_PCIDE     	0x20000
#define CR4_OSXSAVE   	0x40000
#define CR4_PKE       	0x400000
#define CR4_CET       	0x800000

#define CPUID_0   0x0
#define CPUID_1   0x1
#define CPUID_1_ECX_SSE3                (1u << 0)
#define CPUID_1_ECX_PCLMULQDQ         	(1u << 1)
#define CPUID_1_ECX_DTES64            	(1u << 2)
#define CPUID_1_ECX_MONITOR           	(1u << 3)
#define CPUID_1_ECX_DS_CPL            	(1u << 4)
#define CPUID_1_ECX_VMX               	(1u << 5)
#define CPUID_1_ECX_SMX               	(1u << 6)
#define CPUID_1_ECX_EIST              	(1u << 7)
#define CPUID_1_ECX_TM2               	(1u << 8)
#define CPUID_1_ECX_SSSE3             	(1u << 9)
#define CPUID_1_ECX_CNXT_ID           	(1u << 10)
#define CPUID_1_ECX_SDBG              	(1u << 11)
#define CPUID_1_ECX_FMA               	(1u << 12)
#define CPUID_1_ECX_CMPXCHG16B        	(1u << 13)
#define CPUID_1_ECX_XTPR              	(1u << 14)
#define CPUID_1_ECX_PDCM              	(1u << 15)
#define CPUID_1_ECX_PCID              	(1u << 17)
#define CPUID_1_ECX_DCA               	(1u << 18)
#define CPUID_1_ECX_SSE4_1            	(1u << 19)
#define CPUID_1_ECX_SSE4_2            	(1u << 20)
#define CPUID_1_ECX_X2APIC            	(1u << 21)
#define CPUID_1_ECX_MOVBE             	(1u << 22)
#define CPUID_1_ECX_POPCNT            	(1u << 23)
#define CPUID_1_ECX_TSC_DEADLINE      	(1u << 24)
#define CPUID_1_ECX_AESNI             	(1u << 25)
#define CPUID_1_ECX_XSAVE             	(1u << 26)
#define CPUID_1_ECX_OSXSAVE           	(1u << 27)
#define CPUID_1_ECX_AVX               	(1u << 28)
#define CPUID_1_ECX_F16C              	(1u << 29)
#define CPUID_1_ECX_RDRAND            	(1u << 30)
#define CPUID_1_ECX_HYPERVISOR        	(1u << 31)

#define CPUID_1_EDX_MSR		0x20
#define CPUID_1_EDX_PAE		0x40
#define CPUID_1_EDX_APIC	0x200

#define CPUID_EXT0		0x80000000
#define CPUID_EXT1		0x80000001
#define CPUID_EXT1_ECX_SVM	0x4
#define CPUID_EXT1_EDX_64BIT	0x20000000

#define CPUID_EXT2	0x80000002
#define CPUID_EXT3	0x80000003
#define CPUID_EXT4	0x80000004

#define CPUID_EXTA			0x8000000a
#define CPUID_EXTA_EDX_NP		0x1
#define CPUID_EXTA_EDX_SVM_LOCK		0x4
#define CPUID_EXTA_EDX_NRIP_SAVE	0x8
#define CPUID_EXTA_EDX_FLUSH_BY_ASID	0x40
#define CPUID_EXTA_EDX_DECODE_ASSISTS	0x80

#define IA32_MTRR_PHYSBASE0		0x200
#define IA32_MTRR_PHYSBASE0_TYPE_MASK	0xffull
#define IA32_MTRR_PHYSBASE0_PHYSBASE_MASK	0xFFFFFFFFFF000ULL
#define IA32_MTRR_PHYSMASK0		0x201
#define IA32_MTRR_PHYSMASK0_V_BIT	0x800ull
#define IA32_MTRR_PHYSMASK0_PHYSMASK_MASK	0xFFFFFFFFFF000ULL
#define IA32_MTRR_PHYSBASE1	0x202
#define IA32_MTRR_PHYSMASK1	0x203
#define IA32_MTRR_PHYSBASE2	0x204
#define IA32_MTRR_PHYSMASK2	0x205
#define IA32_MTRR_PHYSBASE3	0x206
#define IA32_MTRR_PHYSMASK3	0x207
#define IA32_MTRR_PHYSBASE4	0x208
#define IA32_MTRR_PHYSMASK4	0x209
#define IA32_MTRR_PHYSBASE5	0x20a
#define IA32_MTRR_PHYSMASK5	0x20b
#define IA32_MTRR_PHYSBASE6	0x20c
#define IA32_MTRR_PHYSMASK6	0x20d
#define IA32_MTRR_PHYSBASE7	0x20e
#define IA32_MTRR_PHYSMASK7	0x20f
#define IA32_MTRR_PHYSBASE8	0x210
#define IA32_MTRR_PHYSMASK8	0x211
#define IA32_MTRR_PHYSBASE9	0x212
#define IA32_MTRR_PHYSMASK9	0x213
#define IA32_MTRR_FIX64K_00000	0x250
#define IA32_MTRR_FIX16K_80000	0x258
#define IA32_MTRR_FIX16K_A0000	0x259
#define IA32_MTRR_FIX4K_C0000	0x268
#define IA32_MTRR_FIX4K_C8000	0x269
#define IA32_MTRR_FIX4K_D0000	0x26a
#define IA32_MTRR_FIX4K_D8000	0x26b
#define IA32_MTRR_FIX4K_E0000	0x26c
#define IA32_MTRR_FIX4K_E8000	0x26d
#define IA32_MTRR_FIX4K_F0000	0x26e
#define IA32_MTRR_FIX4K_F8000	0x26f
#define IA32_PAT		0x277

#define IA32_EFER 0xc0000080
#define IA32_EFER_SCE (1 << 0)
#define IA32_EFER_LME (1 << 8)
#define IA32_EFER_LMA (1 << 10)
#define IA32_EFER_NXE (1 << 11)
#define IA32_EFER_SVME (1 << 12)
#define IA32_EFER_LMSLE (1 << 13)
#define IA32_EFER_FFXSR (1 << 14)
#define IA32_EFER_TCE (1 << 15)

#define IA32_FMASK		0xc0000084
#define IA32_FS_BASE		0xc0000100
#define IA32_GS_BASE		0xc0000101
#define IA32_KERNEL_GS_BASE	0xc0000102
#define AMD_VM_CR		0xc0010114
#define AMD_VM_CR_R_INIT	0x2
#define AMD_VM_CR_DIS_A20M	0x4
#define AMD_VM_CR_LOCK		0x8
#define AMD_VM_CR_SVMDIS	0x10
#define AMD_SYSCFG			0xc0010010
#define AMD_SYSCFG_MTRRFIXDRAMEN	0x40000
#define AMD_SYSCFG_MTRRFIXDRAMMODEN	0x80000
#define AMD_SYSCFG_MTRRTOM2EN		0x200000
#define AMD_SYSCFG_TOM2FORCEMEMTYPEWB	0x400000
#define AMD_TOP_MEM2		0xc001001d
#define AMD_TOP_MEM2_ADDR_MASK	0xfffffff800000ULL
#define AMD_VM_HSAVE_PA		0xc0010117


#define IA32_APIC_BASE 0x1b
#define IA32_APIC_BASE_ENABLE_X2APIC 0x400
#define IA32_APIC_BASE_APIC_GLOBAL_ENABLE 0x800
#define IA32_APIC_BASE_APIC_BASE_MASK 0xfffffffffffff000ull

#define IA32_FEATURE_CONTROL	0x3a
#define IA32_FEATURE_CONTROL_LOCK	0x1
#define IA32_FEATURE_CONTROL_VMXON	0x4

#define EFLAGS_IF (1 << 9)


#define PTE_P   (1 << 0)
#define PTE_W   (1 << 1)
#define PTE_U   (1 << 2)
#define PTE_PWT (1 << 3)
#define PTE_PCD (1 << 4)
#define PTE_A   (1 << 5)
#define PTE_D   (1 << 6)
#define PTE_G   (1 << 8)
#define PTE_XD  (1ull << 63)

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

#define PHYSMAX_SHIFT	46

#define PIDX(_level, _addr) (((_addr) >> (12 + ((_level) - 1) * 9)) & 0x1ff)

// Direct mapping offset: 0xffff800000000000 - 0xffffc00000000000

#define KLINK_OFFSET    ULL(0xffff800000000000)
#define KERNLINK        ULL(0xffff800000020000)
#define KERNLINK_PA     ULL(0x20000)

#define PAGE_OFFSET	KLINK_OFFSET

#define DPL_KERNEL	0x0
#define DPL_USER	0x3

#define STA_X	0x8
#define STA_W	0x2
#define STA_R	0x2

#define STA_TSSA	0x9

#define SEG_NULL	0x0
#define SEG_KCODE32	0x1
#define SEG_KDATA32	0x2
#define SEG_KCODE	0x3
#define SEG_KDATA	0x4
#define SEG_UCODE	0x5
#define SEG_UDATA	0x6
#define SEG_TSS		0x7
#define NR_SEG		9

#ifndef __ASSEMBLER__

#include <kernel.h>

#define NR_INTERRUPT	256
#define INT_NMI		2

#define E_DE 0x0
#define E_DB 0x1
#define E_BP 0x3
#define E_OF 0x4
#define E_BR 0x5
#define E_UD 0x6
#define E_NM 0x7
#define E_DF 0x8
#define E_TS 0xa
#define E_NP 0xb
#define E_SS 0xc
#define E_GP 0xd
#define E_PF 0xe

enum gatetype {
	GATEDESC_64_INTR = 0xe,
	GATEDESC_64_TRAP = 0xf,
};

/*
 * x86 Gate Descriptor
 */
struct gatedesc {
	u16 offset_0_15;
	u16 sel;
	u8 ist : 3;
	u8 _rsrv0 : 5;
	u8 gatetype : 4;
	u8 _zero : 1;
	u8 dpl : 2;
	u8 p : 1;
	u16 offset_16_31;
	u32 offset_32_63;
	u32 _rsrv1;
} PACKED;

struct trapframe {
	u64 rax; u64 rbx; u64 rcx; u64 rdx;
	u64 rbp; u64 rsi; u64 rdi; u64 r8;
	u64 r9; u64 r10; u64 r11; u64 r12;
	u64 r13; u64 r14; u64 r15; u64 trapno;
	u64 errcode;
	/* iret */
	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
} PACKED;

struct tss {
  u32 reserved0;
  u64 rsp0;
  u64 rsp1;
  u64 rsp2;
  u64 reserved1;
  u64 ist1;
  u64 ist2;
  u64 ist3;
  u64 ist4;
  u64 ist5;
  u64 ist6;
  u64 ist7;
  u64 reserved2;
  u16 reserved3;
  u16 io_map_base;
} PACKED;

static inline void
hlt(void)
{
	asm volatile ("hlt");
}

static inline void
outb (u16 port, u8 data)
{
	asm volatile ("outb %0, %1" ::"a"(data), "d"(port));
}

static inline void
outw (u16 port, u16 data)
{
	asm volatile ("outw %0, %1" ::"a"(data), "d"(port));
}

static inline void
outl (u16 port, u32 data)
{
	asm volatile ("outl %0, %1" ::"a"(data), "d"(port));
}

static inline u8
inb (u16 port)
{
	u8 data;
	asm volatile ("inb %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static inline u16
inw (u16 port)
{
	u16 data;
	asm volatile("inw %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static inline u32
inl (u16 port)
{
	u32 data;
	asm volatile("inl %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static inline ulong
rdcr2 (void)
{
	ulong r;
	asm volatile ("movq %%cr2, %0" : "=r"(r));
	return r;
}

static inline ulong
rdcr3 (void)
{
	ulong r;
	asm volatile ("movq %%cr3, %0" : "=r"(r));
	return r;
}

static inline void
wrcr3 (ulong cr3)
{
	asm volatile ("movq %0, %%cr3" :: "r"(cr3));
}

static inline ulong
rdcr4 (void)
{
	ulong r;
	asm volatile ("movq %%cr4, %0" : "=r"(r));
	return r;
}

static inline void
cpuid (u32 ax, u32 cx, u32 *a, u32 *b, u32 *c, u32 *d)
{
	asm volatile ("cpuid" : "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d) : "a" (ax), "c" (cx));
}

static inline void
__rdmsr (u32 reg, u32 *a, u32 *d)
{
	asm volatile ("rdmsr" : "=a"(*a), "=d"(*d) : "c"(reg));
}

static inline u32
rdmsr32 (u32 reg)
{
	u32 a, d;
	__rdmsr(reg, &a, &d);
	return a;
}

static inline u64
rdmsr64 (u32 reg)
{
	u32 a, d;
	__rdmsr(reg, &a, &d);
	return (u64)a | ((u64)d << 32);
}

static inline void
__wrmsr (u32 reg, u32 a, u32 d)
{
	asm volatile("wrmsr" ::"c"(reg), "a"(a), "d"(d));
}

static inline void
wrmsr32 (u32 reg, u32 val)
{
	__wrmsr(reg, val, 0);
}

static inline void
wrmsr64 (u32 reg, u64 val)
{
	u32 a, d;
	a = (u32)val;
	d = (u32)(val >> 32);
	__wrmsr(reg, a, d);
}

static inline bool
irqen (void)
{
	u64 rflags;
	asm volatile("pushfq\n"
		     "pop  %0\n"
		     : "=r"(rflags));

	return rflags & EFLAGS_IF;
}

#endif	/* __ASSEMBLER__ */

#endif
