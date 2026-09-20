#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <irq.h>

static irq irqa[NR_INTERRUPT] = {0};

gatedesc idt[NR_INTERRUPT];
extern ulong allvectors[];
// __vector0xnn_()
#define VECTOR(_n)                                                             \
  void __vector##_n##_(void);                                                  \
  asm(".align 4 \n"                                                            \
      "__vector" #_n "_: \n"                                                   \
      " pushq $0 \n"                                                           \
      " pushq $" #_n "\n"                                                      \
      " jmp   traphandler \n")
// vector with error code
#define VECTORERR(_n)                                                          \
  void __vector##_n##_(void);                                                  \
  asm(".align 4 \n"                                                            \
      "__vector" #_n "_: \n"                                                   \
      " pushq $" #_n "\n"                                                      \
      " jmp   traphandler \n")

VECTOR(0x00); VECTOR(0x01); VECTOR(0x02); VECTOR(0x03);
VECTOR(0x04); VECTOR(0x05); VECTOR(0x06); VECTOR(0x07);
VECTORERR(0x08); VECTOR(0x09); VECTORERR(0x0a); VECTORERR(0x0b);
VECTORERR(0x0c); VECTORERR(0x0d); VECTORERR(0x0e); VECTOR(0x0f);
VECTOR(0x10); VECTORERR(0x11); VECTOR(0x12); VECTOR(0x13);
VECTOR(0x14); VECTORERR(0x15); VECTOR(0x16); VECTOR(0x17);
VECTOR(0x18); VECTOR(0x19); VECTOR(0x1a); VECTOR(0x1b);
VECTOR(0x1c); VECTORERR(0x1d); VECTORERR(0x1e); VECTOR(0x1f);
VECTOR(0x20); VECTOR(0x21); VECTOR(0x22); VECTOR(0x23);
VECTOR(0x24); VECTOR(0x25); VECTOR(0x26); VECTOR(0x27);
VECTOR(0x28); VECTOR(0x29); VECTOR(0x2a); VECTOR(0x2b);
VECTOR(0x2c); VECTOR(0x2d); VECTOR(0x2e); VECTOR(0x2f);
VECTOR(0x30); VECTOR(0x31); VECTOR(0x32); VECTOR(0x33);
VECTOR(0x34); VECTOR(0x35); VECTOR(0x36); VECTOR(0x37);
VECTOR(0x38); VECTOR(0x39); VECTOR(0x3a); VECTOR(0x3b);
VECTOR(0x3c); VECTOR(0x3d); VECTOR(0x3e); VECTOR(0x3f);
VECTOR(0x40); VECTOR(0x41); VECTOR(0x42); VECTOR(0x43);
VECTOR(0x44); VECTOR(0x45); VECTOR(0x46); VECTOR(0x47);
VECTOR(0x48); VECTOR(0x49); VECTOR(0x4a); VECTOR(0x4b);
VECTOR(0x4c); VECTOR(0x4d); VECTOR(0x4e); VECTOR(0x4f);
VECTOR(0x50); VECTOR(0x51); VECTOR(0x52); VECTOR(0x53);
VECTOR(0x54); VECTOR(0x55); VECTOR(0x56); VECTOR(0x57);
VECTOR(0x58); VECTOR(0x59); VECTOR(0x5a); VECTOR(0x5b);
VECTOR(0x5c); VECTOR(0x5d); VECTOR(0x5e); VECTOR(0x5f);
VECTOR(0x60); VECTOR(0x61); VECTOR(0x62); VECTOR(0x63);
VECTOR(0x64); VECTOR(0x65); VECTOR(0x66); VECTOR(0x67);
VECTOR(0x68); VECTOR(0x69); VECTOR(0x6a); VECTOR(0x6b);
VECTOR(0x6c); VECTOR(0x6d); VECTOR(0x6e); VECTOR(0x6f);
VECTOR(0x70); VECTOR(0x71); VECTOR(0x72); VECTOR(0x73);
VECTOR(0x74); VECTOR(0x75); VECTOR(0x76); VECTOR(0x77);
VECTOR(0x78); VECTOR(0x79); VECTOR(0x7a); VECTOR(0x7b);
VECTOR(0x7c); VECTOR(0x7d); VECTOR(0x7e); VECTOR(0x7f);
VECTOR(0x80); VECTOR(0x81); VECTOR(0x82); VECTOR(0x83);
VECTOR(0x84); VECTOR(0x85); VECTOR(0x86); VECTOR(0x87);
VECTOR(0x88); VECTOR(0x89); VECTOR(0x8a); VECTOR(0x8b);
VECTOR(0x8c); VECTOR(0x8d); VECTOR(0x8e); VECTOR(0x8f);
VECTOR(0x90); VECTOR(0x91); VECTOR(0x92); VECTOR(0x93);
VECTOR(0x94); VECTOR(0x95); VECTOR(0x96); VECTOR(0x97);
VECTOR(0x98); VECTOR(0x99); VECTOR(0x9a); VECTOR(0x9b);
VECTOR(0x9c); VECTOR(0x9d); VECTOR(0x9e); VECTOR(0x9f);
VECTOR(0xa0); VECTOR(0xa1); VECTOR(0xa2); VECTOR(0xa3);
VECTOR(0xa4); VECTOR(0xa5); VECTOR(0xa6); VECTOR(0xa7);
VECTOR(0xa8); VECTOR(0xa9); VECTOR(0xaa); VECTOR(0xab);
VECTOR(0xac); VECTOR(0xad); VECTOR(0xae); VECTOR(0xaf);
VECTOR(0xb0); VECTOR(0xb1); VECTOR(0xb2); VECTOR(0xb3);
VECTOR(0xb4); VECTOR(0xb5); VECTOR(0xb6); VECTOR(0xb7);
VECTOR(0xb8); VECTOR(0xb9); VECTOR(0xba); VECTOR(0xbb);
VECTOR(0xbc); VECTOR(0xbd); VECTOR(0xbe); VECTOR(0xbf);
VECTOR(0xc0); VECTOR(0xc1); VECTOR(0xc2); VECTOR(0xc3);
VECTOR(0xc4); VECTOR(0xc5); VECTOR(0xc6); VECTOR(0xc7);
VECTOR(0xc8); VECTOR(0xc9); VECTOR(0xca); VECTOR(0xcb);
VECTOR(0xcc); VECTOR(0xcd); VECTOR(0xce); VECTOR(0xcf);
VECTOR(0xd0); VECTOR(0xd1); VECTOR(0xd2); VECTOR(0xd3);
VECTOR(0xd4); VECTOR(0xd5); VECTOR(0xd6); VECTOR(0xd7);
VECTOR(0xd8); VECTOR(0xd9); VECTOR(0xda); VECTOR(0xdb);
VECTOR(0xdc); VECTOR(0xdd); VECTOR(0xde); VECTOR(0xdf);
VECTOR(0xe0); VECTOR(0xe1); VECTOR(0xe2); VECTOR(0xe3);
VECTOR(0xe4); VECTOR(0xe5); VECTOR(0xe6); VECTOR(0xe7);
VECTOR(0xe8); VECTOR(0xe9); VECTOR(0xea); VECTOR(0xeb);
VECTOR(0xec); VECTOR(0xed); VECTOR(0xee); VECTOR(0xef);
VECTOR(0xf0); VECTOR(0xf1); VECTOR(0xf2); VECTOR(0xf3);
VECTOR(0xf4); VECTOR(0xf5); VECTOR(0xf6); VECTOR(0xf7);
VECTOR(0xf8); VECTOR(0xf9); VECTOR(0xfa); VECTOR(0xfb);
VECTOR(0xfc); VECTOR(0xfd); VECTOR(0xfe); VECTOR(0xff);

#undef VECTOR
#undef VECTORERR

asm(".data \n"
    "allvectors: \n");

#define VENTRY(_n) asm(".quad __vector" #_n "_ \n")

VENTRY(0x00); VENTRY(0x01); VENTRY(0x02); VENTRY(0x03);
VENTRY(0x04); VENTRY(0x05); VENTRY(0x06); VENTRY(0x07);
VENTRY(0x08); VENTRY(0x09); VENTRY(0x0a); VENTRY(0x0b);
VENTRY(0x0c); VENTRY(0x0d); VENTRY(0x0e); VENTRY(0x0f);
VENTRY(0x10); VENTRY(0x11); VENTRY(0x12); VENTRY(0x13);
VENTRY(0x14); VENTRY(0x15); VENTRY(0x16); VENTRY(0x17);
VENTRY(0x18); VENTRY(0x19); VENTRY(0x1a); VENTRY(0x1b);
VENTRY(0x1c); VENTRY(0x1d); VENTRY(0x1e); VENTRY(0x1f);
VENTRY(0x20); VENTRY(0x21); VENTRY(0x22); VENTRY(0x23);
VENTRY(0x24); VENTRY(0x25); VENTRY(0x26); VENTRY(0x27);
VENTRY(0x28); VENTRY(0x29); VENTRY(0x2a); VENTRY(0x2b);
VENTRY(0x2c); VENTRY(0x2d); VENTRY(0x2e); VENTRY(0x2f);
VENTRY(0x30); VENTRY(0x31); VENTRY(0x32); VENTRY(0x33);
VENTRY(0x34); VENTRY(0x35); VENTRY(0x36); VENTRY(0x37);
VENTRY(0x38); VENTRY(0x39); VENTRY(0x3a); VENTRY(0x3b);
VENTRY(0x3c); VENTRY(0x3d); VENTRY(0x3e); VENTRY(0x3f);
VENTRY(0x40); VENTRY(0x41); VENTRY(0x42); VENTRY(0x43);
VENTRY(0x44); VENTRY(0x45); VENTRY(0x46); VENTRY(0x47);
VENTRY(0x48); VENTRY(0x49); VENTRY(0x4a); VENTRY(0x4b);
VENTRY(0x4c); VENTRY(0x4d); VENTRY(0x4e); VENTRY(0x4f);
VENTRY(0x50); VENTRY(0x51); VENTRY(0x52); VENTRY(0x53);
VENTRY(0x54); VENTRY(0x55); VENTRY(0x56); VENTRY(0x57);
VENTRY(0x58); VENTRY(0x59); VENTRY(0x5a); VENTRY(0x5b);
VENTRY(0x5c); VENTRY(0x5d); VENTRY(0x5e); VENTRY(0x5f);
VENTRY(0x60); VENTRY(0x61); VENTRY(0x62); VENTRY(0x63);
VENTRY(0x64); VENTRY(0x65); VENTRY(0x66); VENTRY(0x67);
VENTRY(0x68); VENTRY(0x69); VENTRY(0x6a); VENTRY(0x6b);
VENTRY(0x6c); VENTRY(0x6d); VENTRY(0x6e); VENTRY(0x6f);
VENTRY(0x70); VENTRY(0x71); VENTRY(0x72); VENTRY(0x73);
VENTRY(0x74); VENTRY(0x75); VENTRY(0x76); VENTRY(0x77);
VENTRY(0x78); VENTRY(0x79); VENTRY(0x7a); VENTRY(0x7b);
VENTRY(0x7c); VENTRY(0x7d); VENTRY(0x7e); VENTRY(0x7f);
VENTRY(0x80); VENTRY(0x81); VENTRY(0x82); VENTRY(0x83);
VENTRY(0x84); VENTRY(0x85); VENTRY(0x86); VENTRY(0x87);
VENTRY(0x88); VENTRY(0x89); VENTRY(0x8a); VENTRY(0x8b);
VENTRY(0x8c); VENTRY(0x8d); VENTRY(0x8e); VENTRY(0x8f);
VENTRY(0x90); VENTRY(0x91); VENTRY(0x92); VENTRY(0x93);
VENTRY(0x94); VENTRY(0x95); VENTRY(0x96); VENTRY(0x97);
VENTRY(0x98); VENTRY(0x99); VENTRY(0x9a); VENTRY(0x9b);
VENTRY(0x9c); VENTRY(0x9d); VENTRY(0x9e); VENTRY(0x9f);
VENTRY(0xa0); VENTRY(0xa1); VENTRY(0xa2); VENTRY(0xa3);
VENTRY(0xa4); VENTRY(0xa5); VENTRY(0xa6); VENTRY(0xa7);
VENTRY(0xa8); VENTRY(0xa9); VENTRY(0xaa); VENTRY(0xab);
VENTRY(0xac); VENTRY(0xad); VENTRY(0xae); VENTRY(0xaf);
VENTRY(0xb0); VENTRY(0xb1); VENTRY(0xb2); VENTRY(0xb3);
VENTRY(0xb4); VENTRY(0xb5); VENTRY(0xb6); VENTRY(0xb7);
VENTRY(0xb8); VENTRY(0xb9); VENTRY(0xba); VENTRY(0xbb);
VENTRY(0xbc); VENTRY(0xbd); VENTRY(0xbe); VENTRY(0xbf);
VENTRY(0xc0); VENTRY(0xc1); VENTRY(0xc2); VENTRY(0xc3);
VENTRY(0xc4); VENTRY(0xc5); VENTRY(0xc6); VENTRY(0xc7);
VENTRY(0xc8); VENTRY(0xc9); VENTRY(0xca); VENTRY(0xcb);
VENTRY(0xcc); VENTRY(0xcd); VENTRY(0xce); VENTRY(0xcf);
VENTRY(0xd0); VENTRY(0xd1); VENTRY(0xd2); VENTRY(0xd3);
VENTRY(0xd4); VENTRY(0xd5); VENTRY(0xd6); VENTRY(0xd7);
VENTRY(0xd8); VENTRY(0xd9); VENTRY(0xda); VENTRY(0xdb);
VENTRY(0xdc); VENTRY(0xdd); VENTRY(0xde); VENTRY(0xdf);
VENTRY(0xe0); VENTRY(0xe1); VENTRY(0xe2); VENTRY(0xe3);
VENTRY(0xe4); VENTRY(0xe5); VENTRY(0xe6); VENTRY(0xe7);
VENTRY(0xe8); VENTRY(0xe9); VENTRY(0xea); VENTRY(0xeb);
VENTRY(0xec); VENTRY(0xed); VENTRY(0xee); VENTRY(0xef);
VENTRY(0xf0); VENTRY(0xf1); VENTRY(0xf2); VENTRY(0xf3);
VENTRY(0xf4); VENTRY(0xf5); VENTRY(0xf6); VENTRY(0xf7);
VENTRY(0xf8); VENTRY(0xf9); VENTRY(0xfa); VENTRY(0xfb);
VENTRY(0xfc); VENTRY(0xfd); VENTRY(0xfe); VENTRY(0xff);

#undef VENTRY

asm (".text \n");

static inline void
loadidt(gatedesc *idt, ulong sz)
{
  volatile u16 t[5];
  t[0] = (u16)sz - 1;
  t[1] = (u16)(ulong)idt;
  t[2] = (u16)((ulong)idt >> 16);
  t[3] = (u16)((ulong)idt >> 32);
  t[4] = (u16)((ulong)idt >> 48);
  asm volatile("lidt (%0)" :: "r" (t));
}

static inline void
setgate (gatedesc *desc, ulong offset, u16 sel, gatetype type, u8 dpl)
{
  desc->offset_0_15 = (u16)offset;
  desc->sel = sel;
  desc->ist = 0;
  desc->_rsrv0 = 0;
  desc->gatetype = type;
  desc->_zero = 0;
  desc->dpl = dpl;
  desc->p = 1;
  desc->offset_16_31 = (u16)(offset >> 16);
  desc->offset_32_63 = (u32)(offset >> 32);
  desc->_rsrv1 = 0;
}

void
trapinit(void)
{
  for (uint i = 0; i < NR_INTERRUPT; i++)
    setgate(idt + i, allvectors[i], SEG_KCODE << 3, GATEDESC_64_INTR, DPL_KERNEL);
  setgate(idt + 0x80, allvectors[0x80], SEG_KCODE << 3, GATEDESC_64_TRAP, DPL_USER);
  loadidt(idt, sizeof idt);
}

static void
pagefault (trapframe *tf)
{
  panic("pf %p", rdcr2());
}

static void
gpfault(trapframe *tf)
{
  panic("gp");
}

void
newirq(int irqno, void (*eoi)(irq*), int (*handle)(irq*))
{
  irq *i;
  if (irqno >= NR_INTERRUPT)
    return;
  i = &irqa[irqno];
  i->irqno = irqno;
  i->eoi = eoi;
  i->handle = handle;
}

static int
irqhandle(trapframe *tf)
{
  irq *intr;
  if (tf->trapno >= NR_INTERRUPT)
    panic("irqno!?");
  intr = &irqa[tf->trapno];
  if (!intr->eoi)
    return -1;
  intr->eoi(intr);
  if (intr->handle)
    return intr->handle(intr);
  else
    return 0;
}

void
trap(trapframe *tf)
{
  int err = 0;
  switch (tf->trapno) {
    case E_PF:
      pagefault (tf);
      break;
    case E_GP:
      gpfault(tf);
      break;
    case 0x80:
      // syscall (tf);
      break;
    default:
      err = irqhandle(tf);
      if (err < 0)
        panic("irq %d", tf->trapno);
      break;
  }
}

proc *
__cswitch (context *prev, context *next, proc *pprev)
{
  return pprev;
}
