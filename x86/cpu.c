#include <kernel.h>
#include <string.h>
#include <panic.h>
#include "arch.h"
#include "cpu.h"

#define KPREFIX     "x86cpu:"

#include <printk.h>

extern X86cpu __initdata_x86cpu_s[];
extern X86cpu __initdata_x86cpu_e[];

static X86cpu *x86cpu;

static const char *intelid[] = {
        "GenuineIntel",
        NULL,
};

static const char *amdid[] = {
        "AuthenticAMD",
        "AMDisbetter!",
        NULL,
};

static void
x86amdinit (X86cpu *cpu)
{
        char procstr[48] = {0};

        cpuid (CPUID_EXT2,
               (u32 *)procstr, (u32 *)(procstr+4), (u32 *)(procstr+8), (u32 *)(procstr+12));
        cpuid (CPUID_EXT3,
               (u32 *)(procstr+16), (u32 *)(procstr+20), (u32 *)(procstr+24), (u32 *)(procstr+28));
        cpuid (CPUID_EXT4,
               (u32 *)(procstr+32), (u32 *)(procstr+36), (u32 *)(procstr+40), (u32 *)(procstr+44));

        log ("Processor: %s\n", procstr);
}

static X86cpu *
findx86cpu (char *vendor)
{
        const char **id;

        for (X86cpu *c = __initdata_x86cpu_s; c < __initdata_x86cpu_e; c++)
        {
                id = c->id;
                while (*id)
                {
                        if (!strcmp (*id, vendor))
                                return c;
                        id++;
                }
        }
        panic ("x86 cannot find processor");
}

void
x86cpuinit (void)
{
        char cpuvendor[13] = {0};
        u32 a;

        cpuid (CPUID_0, &a, (u32 *)cpuvendor, (u32 *)(cpuvendor+8), (u32 *)(cpuvendor+4));
        log ("CPU Vendor: %s\n", cpuvendor);

        x86cpu = findx86cpu (cpuvendor);
        log ("Vendor: %s\n", x86cpu->vendor);

        if (x86cpu->init)
                x86cpu->init (x86cpu);
}

DEFINE_X86_CPU (Intel, intelid, NULL);
DEFINE_X86_CPU (AMD, amdid, x86amdinit);
