#include <kernel.h>
#include <panic.h>
#include <kalloc.h>
#include <vm.h>
#include <device.h>
#include "arch.h"
#include "apic.h"

#define KPREFIX     "xapic:"

#include <printk.h>

#define XAPIC_ID        0x020
#define XAPIC_VER       0x030
#define XAPIC_TPR       0x080
#define XAPIC_EOI       0x0b0
#define XAPIC_ESR       0x280
#define XAPIC_ICR_LOW   0x300
#define XAPIC_ICR_HIGH  0x310

static ulong apic_basepa;
static volatile void *apic_base;

static inline u32
xapicread32(u32 off)
{
        return *(volatile u32 *)(apic_base + off);
}

static inline void
xapicwrite32(u32 off, u32 val)
{
        *(volatile u32 *)(apic_base + off) = val;
}

static u32
xapicrd(struct apic *apic, u32 reg)
{
        u32 offset = reg;
        return xapicread32(offset);
}

static void
xapicwr(struct apic *apic, u32 reg, u32 val)
{
        u32 offset = reg;
        xapicwrite32(offset, val);
        // wait for completion
        xapicread32(XAPIC_ID);
}

static void
enxapic(void)
{
        ulong apicbase = rdmsr64(IA32_APIC_BASE);

        if (apicbase & IA32_APIC_BASE_APIC_GLOBAL_ENABLE)
                return;

        apicbase |= IA32_APIC_BASE_APIC_GLOBAL_ENABLE;
        wrmsr64 (IA32_APIC_BASE, apicbase);
}

static void
xapicsendipi(struct apic *apic, int id)
{
        ;
}

static u32
apic_baseaddr(void)
{
        ulong apicbase = rdmsr64 (IA32_APIC_BASE);
        return (apicbase & IA32_APIC_BASE_APIC_BASE_MASK);
}

static int
xapicprobe(struct device *dev, struct apic *apic)
{
        struct iomem *iomem;

        enxapic();
        iomem = iomap(dev, apic_baseaddr(), PAGESIZE);
        if (!iomem)
                return -1;

        apic_basepa = apic_baseaddr();
        apic_base = iomem->base;
        return 0;
}

static struct apic_if xapicops = {
        .probe    = xapicprobe,
        .read     = xapicrd,
        .write    = xapicwr,
        .sendipi  = xapicsendipi,
};

static bool
xapicsupported(void)
{
        u32 a, b, c, d;
        cpuid(CPUID_1, &a, &b, &c, &d);
        return !!(d & CPUID_1_EDX_APIC);
}

void
xapicinit(uint id)
{
        if (!xapicsupported())
                return;
        log("Kernel use xapic\n");
        apicinit(id, &xapicops);
}
