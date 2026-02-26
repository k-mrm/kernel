#include <acpi.h>
#include <hpet.h>
#include <kernel.h>
#include <panic.h>
#include <string.h>
#include <x86/apic.h>
#include <x86/ioapic.h>
#include <x86/mm.h>

#define KPREFIX "acpi:"

#include <printk.h>

static Rsdp *rsdp;
static Xsdp *xsdp;

static int rsdpver;
static Rsdt *rsdt;
static Xsdt *xsdt;

#define FOREACH_RSDT(_rsdt, _e, _i)                                            \
  for (_e = rsdtnext((_rsdt), &(_i)); _e; _e = rsdtnext((_rsdt), &(_i)))

#define FOREACH_XSDT(_xsdt, _e, _i)                                            \
  for (_e = xsdtnext((_xsdt), &(_i)); _e; _e = xsdtnext((_xsdt), &(_i)))

static void *rsdtnext(Rsdt *rsdt, uint *itr) {
  SdtHeader *header = (SdtHeader *)rsdt;
  uint nentry;
  uint i = *itr;

  nentry = (header->length - sizeof(*header)) / sizeof(u32);
  if (i < nentry) {
    (*itr)++;
    return P2V(rsdt->entry[i]);
  } else {
    return NULL;
  }
}

static void *xsdtnext(Xsdt *xsdt, uint *itr) {
  SdtHeader *header = (SdtHeader *)xsdt;
  uint nentry;
  uint i = *itr;

  nentry = (header->length - sizeof(*header)) / sizeof(u64);
  if (i < nentry) {
    (*itr)++;
    return P2V(xsdt->entry[i]);
  } else {
    return NULL;
  }
}

static bool rsdpcheck(Rsdp *rsdp) {
  trace("%s\n", rsdp->signature);
  return !strncmp(rsdp->signature, "RSD PTR ", 8);
}

static void *findrsdt(char *sig) {
  SdtHeader *ent;
  uint i = 0;

  FOREACH_RSDT(rsdt, ent, i) {
    if (!strncmp(ent->signature, sig, 4))
      return ent;
  }
  return NULL;
}

static void *findxsdt(char *sig) {
  SdtHeader *ent;
  uint i;

  FOREACH_XSDT(xsdt, ent, i) {
    if (!strncmp(ent->signature, sig, 4))
      return ent;
  }
  return NULL;
}

static void rsdtdump(void) {
  SdtHeader *ent;
  uint i;

  trace("ACPI RSDT:\n");
  FOREACH_RSDT(rsdt, ent, i) {
    trace("\t%.4s (%s %s)\n", ent->signature, ent->oemid, ent->oemtableid);
  }
}

static void xsdtdump(void) {
  SdtHeader *ent;
  uint i = 0;

  trace("ACPI XSDT:\n");
  FOREACH_XSDT(xsdt, ent, i) {
    trace("\t%.4s (%s %s)\n", ent->signature, ent->oemid, ent->oemtableid);
  }
}

static void acpidump(void) {
  if (rsdpver == 1)
    return rsdtdump();
  else if (rsdpver == 2)
    return xsdtdump();
}

static void *acpifind(char *sig) {
  if (rsdpver == 1)
    return findrsdt(sig);
  else if (rsdpver == 2)
    return findxsdt(sig);
  else
    return NULL;
}

static void acpihpetinit(void) {
  HpetAcpi *hpet = acpifind("HPET");

  if (!hpet)
    return;

  if (hpet->address.space != ACPI_SPACE_SYSTEM_MEMORY) {
    log("HPET address space is not memory\n");
    return;
  }
  if (!hpet->address.address) {
    log("HPET address is invalid\n");
    return;
  }
  log("HPET%d register @%p\n", hpet->number, hpet->address.address);
  hpetinit(hpet->address.address, hpet->number);
}

static void apicparseioapic(MadtIoapic *ioapic) {
  log("IOAPIC%d at %p\n", ioapic->ioapicid, ioapic->ioapicaddr);

  ioapicinit(ioapic->ioapicid, ioapic->ioapicaddr, ioapic->intrbase);
}

static void apicparselocalx2apic(MadtLocalx2apic *x2apic) {
  log("x2apic Processor %d found %d %d\n", x2apic->x2apicid, x2apic->acpiid,
      x2apic->flags);

  x2apicinit(x2apic->x2apicid);
}

static void apicparselocalapic(MadtLocalapic *apic) {
  if (!(apic->flags & 1))
    return;
  log("Processor %d(%d) found\n", apic->procid, apic->apicid);
  xapicinit(apic->apicid);
}

static void acpimadtinit(void) {
  Madt *madt;
  MadtEntry *ent;
  ulong len;

  madt = acpifind("APIC");
  if (!madt)
    return;

  len = ((SdtHeader *)madt)->length;
  for (ent = madt->table; (ulong)ent < (ulong)madt + len;
       ent = (MadtEntry *)((ulong)ent + ent->length)) {
    switch (ent->type) {
    case APIC_TYPE_LOCALAPIC:
      apicparselocalapic((MadtLocalapic *)ent);
      break;
    case APIC_TYPE_IOAPIC:
      apicparseioapic((MadtIoapic *)ent);
      break;
    case APIC_TYPE_LOCAL_X2APIC:
      apicparselocalx2apic((MadtLocalx2apic *)ent);
      break;
    }
  }
}

void regxsdp(void *x) {
  if (!x)
    warn("null xsdp");
  if (rsdpcheck(x))
    xsdp = (Xsdp *)x;
}

void regrsdp(void *r) {
  if (!r)
    warn("null rsdp");
  if (rsdpcheck(r))
    rsdp = (Rsdp *)r;
}

void acpiinit(void) {
  if (xsdp && rsdpcheck(xsdp)) {
    rsdpver = 2;
    goto found;
  }

  if (rsdp && rsdpcheck(rsdp)) {
    rsdpver = 1;
    goto found;
  }
  panic("no acpi table");

found:
  log("found acpi table version%d\n", rsdpver);
  log("acpi (%s %s) rsdt %p\n", rsdp->signature, rsdp->oemid,
      rsdp->rsdtaddress);

  if (rsdpver == 1)
    rsdt = P2V(rsdp->rsdtaddress);
  else if (rsdpver == 2)
    xsdt = P2V(xsdp->xsdtaddress);
  acpidump();
  acpimadtinit();
  acpihpetinit();
}
