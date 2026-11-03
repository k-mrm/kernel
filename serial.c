#include <kernel.h>
#include <asm.h>
#include <sys.h>

#define COM1 0x3f8
#define COM2 0x2f8

#define DATA 0x0
#define IER 0x1
#define DLL 0x0
#define DLH 0x1
#define IIR 0x2
#define FCR 0x2
#define LCR 0x3
#define MCR 0x4
#define LSR 0x5
#define MSR 0x6

#define DLAB 0x80

static bool
empty(void)
{
  return inb(COM1 + LSR) & 0x20;
}

static bool
coming(void)
{
  return inb(COM1 + LSR) & 0x1;
}

void
serial_putc(char c)
{
  while (!empty())
    ;
  outb(COM1 + DATA, c);
}

int
serialgetc(void)
{
  if (coming())
    return inb(COM1 + DATA);
  else
    return -1;
}

static int
serialintr(irq *irq)
{
  int c;
  while ((c = serialgetc()) >= 0)
    consintr(c);
  return 0;
}

void
serial_init(void)
{
  u16 div = 1;  // baudrate 115200
  outb(COM1 + IER, 0x0);
  outb(COM1 + FCR, 0x0);
  outb(COM1 + LCR, 0x80);
  outb(COM1 + DLL, div & 0xff);
  outb(COM1 + DLH, (div >> 8) & 0xff);
  outb(COM1 + MCR, 0xb);
  outb(COM1 + LCR, 0x3);
  newirq(ioapicen(4), lapiceoi, serialintr, NULL);
  outb(COM1 + IER, 0x1);
}
