#include "arch.h"
#include "pic-8259a.h"

#define PIC1         0x20
#define PIC2         0xa0

// disable PIC8259A
void
initpic8259a (void)
{
  outb (PIC1 + 1, 0xff);
  outb (PIC2 + 1, 0xff);
}
