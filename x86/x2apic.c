#include "apic.h"
#include "arch.h"
#include <kalloc.h>
#include <kernel.h>
#include <panic.h>
#include <vm.h>

#define KPREFIX "x2apic:"

#include <printk.h>

static void x2apicsendipi(struct apic *apic, int id) { ; }

static struct apic_if x2apicops = {
    .probe = NULL,
    .read = NULL,
    .write = NULL,
    .sendipi = x2apicsendipi,
};

static bool x2apicsupported(void) {
  u32 a, b, c, d;
  cpuid(CPUID_1, &a, &b, &c, &d);
  return !!(c & CPUID_1_ECX_X2APIC);
}

void x2apicinit(uint id) {
  if (!x2apicsupported())
    return;
  log("Kernel use x2apic\n");
  apicinit(id, &x2apicops);
}
