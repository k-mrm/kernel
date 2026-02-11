#ifndef _DEVICE_H
#define _DEVICE_H

#include <kernel.h>

struct device;

struct driver
{
	char    *name;
	char    *description;

	int     (*probe)(struct device *);
	void    (*disconnect)(struct device *);
	int     (*reconnect)(struct device *);
	void    (*suspend)(struct device *);
	void    (*resume)(struct device *);

	char    *param;
};

struct device
{
	char *type;
	char name[40];

	struct list iomem;

	struct tree node;

	struct irqchip *irqchip;	// nullable

	struct driver *driver;
};

struct iomem {
        volatile void *base;
        ulong pbase;
        uint size;
	struct list n;
};

struct iomem *iomap(struct device *dev, ulong base, uint size);
struct device *parent_device(struct device *dev);
void dev_probe(char *type);
void lsdev(void);
int new_device(struct device *dev, char *ty, char *name, struct driver *drv, struct tree *parent);
int dev_resume(struct device *dev);
int dev_suspend(struct device *dev);
void *dev_traverse(char *ty, void *(*devcb)(struct device *, void *), void *arg);
void *dev_traverse_cpu(char *ty, void *(*devcb)(struct device *, void *), void *arg);

#endif  // _DEVICE_H
