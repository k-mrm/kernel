#include <kernel.h>
#include <printk.h>
#include <multiboot.h>
#include <sysmem.h>
#include <x86/mm.h>

void
mb2parsebootinfo(MultiBootInfo *mb)
{
  MultiBootTag *tag;
  
  if (!mb)
    return;

  sysrsrvmem(V2P(mb), mb->totalsize);
  
  for (tag = (MultiBootTag *)((char *)mb + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (MultiBootTag *)((char *)tag + ((tag->size + 7) & ~7))) {
    switch (tag->type) {
    case MULTIBOOT_TAG_TYPE_CMDLINE: {
      MultiBootTagString *cmd = (MultiBootTagString *)tag;
      break;
    }
    case MULTIBOOT_TAG_TYPE_BOOTDEV: {
      MultiBootTagBootdev *dev = (MultiBootTagBootdev *)tag;
      break;
    }
    case MULTIBOOT_TAG_TYPE_MMAP: {
      MultiBootMmapEntry *e;
      MultiBootTagMmap *mmap = (MultiBootTagMmap *)tag;
      
      for (e = mmap->entries;
     (char *)e < (char *)mmap + mmap->size;
     e = (MultiBootMmapEntry *)((char *)e + mmap->entrysize)) {
  if (e->type == MULTIBOOT_MEMORY_AVAILABLE)
    sysavailmem (e->addr, e->len);
  else if (e->type == MULTIBOOT_MEMORY_RESERVED)
    sysrsrvmem (e->addr, e->len);
      }
      break;
    }
    case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
      MultiBootTagAcpiOld *acpi = (MultiBootTagAcpiOld *)tag;
      regrsdp (&acpi->rsdp);
      break;
    }
    case MULTIBOOT_TAG_TYPE_ACPI_NEW: {
      MultiBootTagAcpiNew *acpi = (MultiBootTagAcpiNew *)tag;
      regxsdp (&acpi->xsdp);
      break;
    }
    default:
      break;
    }
  }
}
