#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

#include <kernel.h>

// MultiBoot1/MultiBoot2

typedef struct MultiBootInfo    MultiBootInfo;
typedef struct MultiBoot1Info   MultiBoot1Info;

// MultiBoot1
struct MultiBoot1Info
{
  u32 flags;            // offset 0
  u32 memlower;         // offset 4
  u32 memupper;         // offset 8
  u32 bootdevice;       // offset 12
  u32 cmdline;          // offset 16
  u32 modscount;        // offset 20
  u32 modsaddr;         // offset 24
  u32 syms[4];          // offset 28-40
  u32 mmaplen;          // offset 44
  u32 mmapaddr;         // offset 48
  u32 driveslen;        // offset 52
  u32 drivesaddr;       // offset 56
  u32 configtable;      // offset 60
  u32 bootloadername;   // offset 64
  u32 apmtable;         // offset 68
  u32 vbecontrolinfo;   // offset 72
  u32 vbemodeinfo;      // offset 76
  u16 vbemode;          // offset 80
  u16 vbeinterfaceseg;  // offset 82
  u16 vbeinterfaceoff;  // offset 84
  u16 vbeinterfacelen;  // offset 86
} PACKED;

struct MultiBootInfo
{
  u32 totalsize;
  u32 _rsvd;
} PACKED;

#define MULTIBOOT_TAG_ALIGN                  8
#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_CMDLINE           1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT_TAG_TYPE_MODULE            3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT_TAG_TYPE_MMAP              6
#define MULTIBOOT_TAG_TYPE_VBE               7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER       8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS      9
#define MULTIBOOT_TAG_TYPE_APM               10
#define MULTIBOOT_TAG_TYPE_EFI32             11
#define MULTIBOOT_TAG_TYPE_EFI64             12
#define MULTIBOOT_TAG_TYPE_SMBIOS            13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD          14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW          15
#define MULTIBOOT_TAG_TYPE_NETWORK           16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP          17
#define MULTIBOOT_TAG_TYPE_EFI_BS            18
#define MULTIBOOT_TAG_TYPE_EFI32_IH          19
#define MULTIBOOT_TAG_TYPE_EFI64_IH          20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR    21

typedef struct MultiBootTag           MultiBootTag;
typedef struct MultiBootTagString     MultiBootTagString;
typedef struct MultiBootTagBootdev    MultiBootTagBootdev;
typedef struct MultiBootTagMmap       MultiBootTagMmap;
typedef struct MultiBootMmapEntry     MultiBootMmapEntry;
typedef struct MultiBootTagAcpiOld    MultiBootTagAcpiOld;
typedef struct MultiBootTagAcpiNew    MultiBootTagAcpiNew;

struct MultiBootTag
{
  u16 type;
  u16 flags;
  u32 size;
} PACKED;

struct MultiBootTagString
{
  u32     type;
  u32     size;
  char    string[0];
} PACKED;

struct MultiBootTagBootdev
{
  u32 type;
  u32 size;
  u32 biosdev;
  u32 slice;
  u32 part;
} PACKED;

struct MultiBootMmapEntry
{
  u64 addr;
  u64 len;
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
  u32 type;
  u32 zero;
} PACKED;

struct MultiBootTagMmap
{
  u32                type;
  u32                size;
  u32                entrysize;
  u32                entryversion;
  MultiBootMmapEntry entries[0];
} PACKED;

struct MultiBootTagAcpiOld
{
  u32     type;
  u32     size;
  u8      rsdp;
} PACKED;

struct MultiBootTagAcpiNew
{
  u32     type;
  u32     size;
  u8      xsdp;
} PACKED;

void mb2parsebootinfo (MultiBootInfo *mb);

#endif  // _MULTIBOOT_H
