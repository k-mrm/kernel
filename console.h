#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <kernel.h>
#include <device.h>
#include <proc.h>
#include <irq.h>

struct console;

struct console_if {
  int (*write)(struct console *cs, const char *buf, uint n);
  int (*read)(struct console *cs);
  int (*csirq)(struct console *cs, struct irq *irq);
};

struct cbuf {
  char data[2048];
  int read;
  int write;
};

struct flipbuf {
  struct cbuf *cbuf;
  struct cbuf buf[2];
};

struct console {
  struct device dev;
  struct console_if *ops;

  struct flipbuf buf;
  struct chan chan;
};

#define dev_console(_d)         container_of(_d, struct console, dev)

extern struct console *console;

int probe_console(struct console *cs);
int console_irq(struct irq *irq);

#endif  // _CONSOLE_H
