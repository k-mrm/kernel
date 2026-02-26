#include <kernel.h>
#include <device.h>
#include <printk.h>
#include <vm.h>
#include <string.h>
#include <cpu.h>
#include <kalloc.h>

struct dev_trav_arg {
  void *(*devcb)(struct device *, void *);
  void *arg;
  char *ty;
};

ROOT(devtree);

int
new_device(struct device *dev, char *ty, char *name, struct driver *drv, struct tree *parent)
{
  if (!drv)
    return -1;

  inittree(&dev->node);

  dev->type = ty;
  if (parent)
    new_child(parent, &dev->node);
  else
    new_child(&devtree, &dev->node);
  strcpy(dev->name, name);
  initlist(&dev->iomem);
  dev->driver = drv;

  return 0;
}

struct device *
parent_device(struct device *dev)
{
  struct tree *node = &dev->node;
  struct tree *pnode = node->par;

  if (pnode == &devtree)
    return NULL;
  else
    return TREE_ENTRY(pnode, struct device, node);
}

static void *
__devtree_traverse(struct tree *node, void *arg)
{
  struct device *dev = TREE_ENTRY(node, struct device, node); 
  struct dev_trav_arg *da = arg;
  void *(*cb)(struct device *, void *);
  void *cb_arg;
  void *ret;

  if (da->ty == NULL || strcmp(dev->type, da->ty) == 0) {
    cb = da->devcb;
    cb_arg = da->arg;

    ret = (*cb)(dev, cb_arg);
    if (ret)
      return ret;
  }

  return NULL;
}

void *
dev_traverse(char *ty, void *(*devcb)(struct device *, void *), void *arg)
{
  struct dev_trav_arg da = {
    .devcb = devcb,
    .arg = arg,
    .ty = ty,
  };

  return tree_dfs(&devtree, __devtree_traverse, &da);
}

void *
dev_traverse_cpu(char *ty, void *(*devcb)(struct device *, void *), void *arg)
{
  struct cpu *cpu = mycpu();
  struct dev_trav_arg da = {
    .devcb = devcb,
    .arg = arg,
    .ty = ty,
  };

  return tree_dfs(&cpu->devtree, __devtree_traverse, &da);
}

static void *
__dev_probe(struct device *dev, void *_)
{
  log("probe %s: %s\n", dev->type, dev->name);
  if (dev->driver->probe)
    dev->driver->probe(dev);
  log("probe %s: %s OK\n", dev->type, dev->name);

  return NULL;
}

void
dev_probe(char *type)
{
  dev_traverse(type, __dev_probe, NULL);
  dev_traverse_cpu(type, __dev_probe, NULL);
}

static void *
devdump(struct device *dev, void *_)
{
  printk("%s device: %s\n", dev->type, dev->name);

  if (dev->driver) {
    printk("  driver: %s\n", dev->driver->name);
    printk("    %s\n", dev->driver->description);
  }
  return NULL;
}

void
lsdev(void)
{
  dev_traverse(NULL, devdump, NULL);
  dev_traverse_cpu(NULL, devdump, NULL);
}

struct iomem *
iomap(struct device *dev, ulong base, uint size)
{
  struct iomem *iomem;

  iomem = alloc();
  if (!iomem)
    return NULL;

  iomem->base = devmmap(base, size);
  if (!iomem->base)
    goto failed;
  iomem->pbase = base;
  iomem->size = size;

  list_add(&dev->iomem, &iomem->n);

  return iomem;
failed:
  free(iomem);
  return NULL;
}

int
dev_resume(struct device *dev)
{
  return -1;
}

int
dev_suspend(struct device *dev)
{
  return -1;
}
