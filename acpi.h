#ifndef _ACPI_H
#define _ACPI_H

#include <kernel.h>

typedef struct Rsdp Rsdp;
typedef struct Xsdp Xsdp;
typedef struct SdtHeader SdtHeader;
typedef struct Rsdt Rsdt;
typedef struct Xsdt Xsdt;

struct Rsdp {
  char signature[8];
  u8 checksum;
  char oemid[6];
  u8 revision;
  u32 rsdtaddress;
} PACKED;

struct Xsdp {
  Rsdp v1;
  u32 length;
  u64 xsdtaddress;
  u8 xchecksum;
  u8 _rsrv[3];
} PACKED;

struct SdtHeader {
  char signature[4];
  u32 length;
  u8 revision;
  u8 checksum;
  char oemid[6];
  char oemtableid[8];
  u32 oemrevision;
  u32 creatorid;
  u32 creatorrevision;
} PACKED;

struct Rsdt {
  SdtHeader header;
  u32 entry[];
} PACKED;

struct Xsdt {
  SdtHeader header;
  u64 entry[];
} PACKED;

/*
 * FACP
 */
typedef struct Fadt Fadt;

struct Fadt {
  SdtHeader header;
} PACKED;

/*
 * MADT
 */
typedef struct MadtEntry MadtEntry;
typedef struct MadtLocalapic MadtLocalapic;
typedef struct MadtIoapic MadtIoapic;
typedef struct MadtLocalx2apic MadtLocalx2apic;
typedef struct Madt Madt;

#define APIC_TYPE_LOCALAPIC 0
#define APIC_TYPE_IOAPIC 1
#define APIC_TYPE_LOCAL_X2APIC 9

struct MadtEntry {
  u8 type;
  u8 length;
};

struct MadtLocalapic {
  MadtEntry header;

  u8 procid; // Processor ID
  u8 apicid; // Local APIC ID
  u32 flags;
};

struct MadtIoapic {
  MadtEntry header;

  u8 ioapicid;
  u8 _zero;
  u32 ioapicaddr;
  u32 intrbase;
};

struct MadtLocalx2apic {
  MadtEntry header;

  u16 _rsrv;
  u32 x2apicid;
  u32 flags;
  u32 acpiid;
};

struct Madt {
  SdtHeader header;
  u32 lapicaddr;
  u32 flags;
  MadtEntry table[];
} PACKED;

/*
 * General Address Structure
 */
#define ACPI_SPACE_SYSTEM_MEMORY 0x0
#define ACPI_SPACE_SYSTEM_IO 0x1
#define ACPI_SPACE_PCI_CONFIG 0x2
#define ACPI_SPACE_PCI_BAR 0x6

typedef struct Gas Gas;

struct Gas {
  u8 space;
  u8 bitwidth;
  u8 bitoffset;
  u8 accesssize;
  u64 address;
} PACKED;

/*
 * HPET
 */
typedef struct HpetAcpi HpetAcpi;

struct HpetAcpi {
  SdtHeader header;
  u32 hardwareid;
  Gas address;
  u8 number;
  u16 mintick;
  u8 flags;
} PACKED;

void acpiinit(void);

#endif // _ACPI_H
