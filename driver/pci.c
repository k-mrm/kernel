#include <kernel.h>
#include <pci.h>
#include <device.h>
#include <kalloc.h>
#include <module.h>
#include <x86/pci.h>

#define KPREFIX         "PCI:"

#include <printk.h>

static LIST_HEAD(pci_driver);

static int
pci_probe(struct device *dev)
{
  struct pci_device *pci = dev_pci(dev);

  if (pci->driver)
    return pci->driver->probe(pci);
  else
    return -1;
}

static void
pci_suspend(struct device *dev)
{
  ;
}

static void
pci_resume(struct device *dev)
{
  ;
}

static struct driver driver = {
  .name         = "PCI",
  .description  = "PCI(e) generic driver",
  .probe        = pci_probe,
  .suspend      = pci_suspend,
  .resume       = pci_resume,
  .param        = "disable",
};

static struct pci_device *
pcifind(u16 vendor, u16 device)
{
  return NULL;
}

static struct pci_driver *
pci_find_driver(struct pci_device *pci)
{
  struct pci_driver *drv;
  struct pci_id *id;

  LIST_FOREACH (drv, &pci_driver, dn) {
    for (id = drv->id; id->vendor != 0; id++) {
      if (pci->vendorid == id->vendor &&
    pci->deviceid == id->device) {
  log("found driver: %s\n", drv->name);
  return drv;
      }
    }
  }

  return NULL;
}

int
reg_pci_driver(struct pci_driver *drv)
{
  list_add(&pci_driver, &drv->dn);
  return 0;
}

static char *
pciname(struct pci_device *pci, char *name)
{
  sprintf(name, "%02x:%02x.%02x", pci->bus, DEVNO(pci->devfn), FUNCNO(pci->devfn));
  return name;
}

static int
new_pci(int bus, int devfn)
{
  struct pci_device *pci;
  char name[40] = {0};
  
  pci = zalloc();
  if (!pci)
    return -1;

  pci->bus = bus;
  pci->devfn = devfn;
  pci->vendorid = pci_read(pci, PCI_CONFIG_VENDOR_ID, 2);
  pci->deviceid = pci_read(pci, PCI_CONFIG_DEVICE_ID, 2);
  pci->hdrtype = pci_read(pci, PCI_CONFIG_HEADER_TYPE, 1);

  new_device(&pci->dev, "PCI", pciname(pci, name), &driver, NULL /* XXX */);

  pci->driver = pci_find_driver(pci);
  return 0;
}

static void
initpci(void)
{
  int bn, dn, fn;
  u16 v = 0;
  u8 headertype;

  for (bn = 0; bn < 256; bn++)
  for (dn = 0; dn < 32; dn++)
  for (fn = 0; fn < 8; fn++) {
    pci_cfg_read(bn, DEVFN(dn, fn), PCI_CONFIG_VENDOR_ID, sizeof v, (u32 *)&v);
    if (v == 0xffff)
      continue;

    if (new_pci(bn, DEVFN(dn, fn)) < 0)
      warn("PCI? %02x:%02x:%02x\n", bn, dn, fn);

    pci_cfg_read(bn, DEVFN(dn, fn), PCI_CONFIG_HEADER_TYPE,
     sizeof headertype, (u32 *)&headertype);

    if (fn == 0 && !(headertype & PCI_CONFIG_HEADER_TYPE_MULTIFUNCTION))
      break;
  }
}

MODULE_DECL pci = {
  .name           = "pci",
  .description    = "PCI(e) driver",
  .init           = initpci,
  .delete         = NULL,
};
