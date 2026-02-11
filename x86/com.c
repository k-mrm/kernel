#include <kernel.h>
#include <console.h>
#include <irq.h>
#include "arch.h"
#include "com.h"
#include "ioapic.h"

#define COM1    0x3f8
#define COM2    0x2f8

#define DATA  0x0
#define IER   0x1
#define DLL   0x0
#define DLH   0x1
#define IIR   0x2
#define FCR   0x2
#define LCR   0x3
#define MCR   0x4
#define LSR   0x5
#define MSR   0x6

#define DLAB  0x80

struct com
{
	struct console cs;
        u16 port;
        u32 baud;
        int irqno;
};

static struct com com1 = {
        .port = COM1,
        .baud = 115200,
        .irqno = 4,
};

static int
cominit (struct console *cs)
{
        struct com *com = container_of(cs, struct com, cs);
        u16 port = com->port;
        u16 div = 115200 / com->baud;

        // Disable Interrupt
        outb(port + IER, 0x0);
        // Disable FIFO
        outb(port + FCR, 0x0);
        // Baudrate
        outb(port + LCR, 0x80);
        outb(port + DLL, div & 0xff);
        outb(port + DLH, (div >> 8) & 0xff);
        // RTS | DTR
        outb(port + MCR, 0x3);

        // 8n1
        outb(port + LCR, 0x3);

	outb(port + IER, 0x1);

        return 0;
}

static bool
comempty(struct com *com)
{
	return inb(com->port + LSR) & 0x20;
}

static void
comsend(struct com *com, char c)
{
	while (comempty(com) == 0)
		;

	outb(com->port + DATA, c);
}

static char
comgetc(struct com *com)
{
	return inb(com->port + DATA);
}

static bool
com_in(struct com *com)
{
	return inb(com->port + LSR) & 0x1;
}

static void
computc(struct console *cs, char c)
{
	struct com *com = container_of(cs, struct com, cs);

	if (c == '\n')
		comsend(com, '\r');
	comsend(com, c);
}

static int
comwrite(struct console *cs, const char *buf, uint n)
{
	for (uint i = 0; i < n && buf[i]; i++)
		computc(cs, buf[i]);

	return n;
}

static int
comread(struct console *cs)
{
	struct com *com = container_of(cs, struct com, cs);

	if (com_in(com))
		return comgetc(com);
	else
		return -1;
}

static int
comirq(struct console *cs, struct irq *irq)
{
	struct com *com = container_of(cs, struct com, cs);

	return 0;
}

static struct console_if cons = {
	.write = comwrite,
	.read = comread,
	.csirq = comirq,
};

static int
com_probe(struct device *dev)
{
	struct console *cs = dev_console(dev);
	struct com *com = container_of(cs, struct com, cs);
	struct irq *irq;

	cominit(cs);

	cs->ops = &cons;

	irq = dev->irqchip->new_irq(dev->irqchip, dev, com->irqno, console_irq);
	if (!irq)
		return -1;

	return probe_console(cs);
}

static struct driver com_drv = {
	.name		= "COM port",
	.description	= "COM port Driver",
	.probe		= com_probe,
	.suspend	= NULL,
	.resume		= NULL,
	.param		= "disable",
};

void
serialportinit(void)
{
	new_device(&com1.cs.dev, "console", "com", &com_drv, NULL);
	com1.cs.dev.irqchip = &ioapic_chip;
}
