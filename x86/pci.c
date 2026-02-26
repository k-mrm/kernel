#include "arch.h"
#include <kernel.h>
#include <pci.h>
#include <printk.h>

#define PCI_CONFIG_ADDR_PORT 0x0cf8
#define PCI_CONFIG_DATA_PORT 0x0cfc

// static Lock   lock;

#define CONF(_bus, _devfn, _reg)                                               \
  ((1ul << 31) | (((_reg) & 0xf00) << 16) | ((_bus) << 16) |                   \
   ((_devfn) << 8 | ((_reg) & 0xfc)))

int pci_cfg_read(int bus, int devfn, int reg, int size, u32 *v) {
  int ret = 0;
  u16 dport = PCI_CONFIG_DATA_PORT + (reg & 3);

  // acquire lock;
  outl(PCI_CONFIG_ADDR_PORT, CONF(bus, devfn, reg));

  switch (size) {
  case 1:
    *(u8 *)v = inb(dport);
    break;
  case 2:
    *(u16 *)v = inw(dport);
    break;
  case 4:
    *v = inl(dport);
    break;
  default:
    ret = -1;
    break;
  }

  // release lock;

  return ret;
}

int pci_cfg_write(int bus, int devfn, int reg, int size, u32 v) {
  int ret = 0;
  u16 dport = PCI_CONFIG_DATA_PORT + (reg & 3);

  // acquire lock;
  outl(PCI_CONFIG_ADDR_PORT, CONF(bus, devfn, reg));

  switch (size) {
  case 1:
    outb(dport, (u8)v);
    break;
  case 2:
    outw(dport, (u16)v);
    break;
  case 4:
    outl(dport, v);
    break;
  default:
    ret = -1;
    break;
  }

  // release lock;

  return ret;
}
