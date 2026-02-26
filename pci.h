#ifndef _PCI_H
#define _PCI_H

#include <device.h>
#include <kernel.h>

#define DEVFN(_d, _f) (((_d) << 3) | (_f))
#define DEVNO(_devfn) (((_devfn) & 0xf8) >> 3)
#define FUNCNO(_devfn) ((_devfn) & 0x7)

#define PCI_CONFIG_VENDOR_ID 0x00
#define PCI_CONFIG_DEVICE_ID 0x02
#define PCI_CONFIG_COMMAND_INTERRUPT_DISABLE 0x0400
#define PCI_CONFIG_STATUS 0x06
#define PCI_CONFIG_REVISION_ID 0x08
#define PCI_CONFIG_CLASS_CODE 0x09
#define PCI_CONFIG_PROGRAMMING_IF 0x09
#define PCI_CONFIG_SUB_CLASS 0x0a
#define PCI_CONFIG_BASE_CLASS 0x0b
#define PCI_CONFIG_CACHE_LINE_SIZE 0x0c
#define PCI_CONFIG_LATENCY_TIMER 0x0d
#define PCI_CONFIG_HEADER_TYPE 0x0e
#define PCI_CONFIG_HEADER_TYPE_MULTIFUNCTION (1u << 7)
#define PCI_CONFIG_BIST 0x0f

#define PCI_CONFIG_CARDBUS_CIS_POINTER 0x28
#define PCI_CONFIG_SUBSYSTEM_VENDOR_ID 0x2c
#define PCI_CONFIG_SUBSYSTEM_ID 0x2e
#define PCI_CONFIG_EXPANSION_ROM 0x30
#define PCI_CONFIG_CAPABILITY_POINTER 0x34
#define PCI_CONFIG_INTERRUPT_LINE 0x3c
#define PCI_CONFIG_INTERRUPT_PIN 0x3d
#define PCI_CONFIG_MIN_GRANT 0x3e
#define PCI_CONFIG_MAX_LATENCY 0x3f

struct pci_device;

struct pci_id {
  u16 vendor;
  u16 device;
};

struct pci_driver {
  struct driver *driver;
  char *name;
  struct pci_id *id;

  struct list dn;

  int (*probe)(struct pci_device *dev);
};

struct pci_device {
  struct device dev;

  struct iomem *bar[10];
  struct pci_driver *driver;

  u8 bus;
  u8 devfn;
  u16 vendorid;
  u16 deviceid;
  u8 hdrtype;
};

#define dev_pci(_d) container_of((_d), struct pci_device, dev)

int pci_cfg_read(int bus, int devfn, int reg, int size, u32 *v);
int pci_cfg_write(int bus, int devfn, int reg, int size, u32 v);

int new_pci_driver(struct pci_driver *drv);

static inline u32 pci_read(struct pci_device *dev, int reg, int size) {
  u32 v = 0;

  pci_cfg_read(dev->bus, dev->devfn, reg, size, &v);

  return v;
}

static inline void pci_write(struct pci_device *dev, int reg, int size, u32 v) {
  pci_cfg_write(dev->bus, dev->devfn, reg, size, v);
}

#endif // _PCI_H
