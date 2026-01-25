#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <kernel.h>
#include <device.h>
#include <irq.h>

struct console;

struct console_if {
        int (*write)(struct console *cs, const char *buf, uint n);
        int (*read)(struct console *cs, char *buf, uint n);
        int (*csirq)(struct console *cs, struct irq *irq);
};

struct console {
	struct device dev;
	struct console_if *ops;
};

#define dev_console(_d)		container_of(_d, struct console, dev)

extern struct console *console;

int probe_console(struct console *cs);
int console_irq(struct irq *irq);

#endif  // _CONSOLE_H
