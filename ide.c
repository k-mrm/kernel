#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <lock.h>
#include <block.h>
#include <irq.h>
#include <wait.h>

typedef struct idedisk idedisk;
typedef struct idechan idechan;

#define cdata(_c)     ((_c)->reg + 0)
#define cerror(_c)    ((_c)->reg + 1)
#define cnsect(_c)    ((_c)->reg + 2)
#define clbal(_c)     ((_c)->reg + 3)
#define clbam(_c)     ((_c)->reg + 4)
#define clbah(_c)     ((_c)->reg + 5)
#define cdevice(_c)   ((_c)->reg + 6)
#define cstatus(_c)   ((_c)->reg + 7)
#define ccommand(_c)  cstatus(_c)
#define cctl(_c)      ((_c)->reg + 0x206)
#define calt(_c)      cctl(_c)

#define STA_BSY   0x80
#define STA_DRDY  0x40
#define STA_DRQ   0x08
#define CTL_SRST  0x04

#define CMD_READ  0x20
#define CMD_WRITE 0x30

struct idedisk {
  idechan *chan;
  int master;
  bool present;
};

struct idechan {
  spinlock lk;
  waitpoint wait;
  idedisk disk[2];
  u16 reg;
};

static void
dselect(idedisk *d)
{
  idechan *c = d->chan;
  u8 dev = 0xa0;
  if (!d->master)
    dev |= 0x10;
  outb(cdevice(c), dev);
  inb(calt(c));
  // TODO: sleep 400ns
}

static int
diskwait(idedisk *d)
{
  idechan *c = d->chan;
  dselect(d);
  while (inb(cstatus(c)) & STA_BSY)
    ;
  return 0;
}

static bool
diskpresent(idedisk *d)
{
  idechan *c = d->chan;
  dselect(d);
  d->present = false;
  for (int i = 0; i < 1000; i++) {
    if (inb(cstatus(c)) != 0) {
      d->present = true;
      break;
    }
  }
  return d->present;
}

static int
ideintr(irq *irq)
{
  idechan *c = irq->data;
  printk("ideintr\n");
  lock(&c->lk);
  inb(cstatus(c));
  wakeup(&c->wait);
  unlock(&c->lk);
  return 0;
}

static int
idebio(block *b, int write, ulong bno, void *buf)
{
  idedisk *d = b->data;
  idechan *c = d->chan;
  lock(&c->lk);
  diskwait(d);
  outb(cctl(c), 0);
  outb(cnsect(c), 1);
  outb(clbal(c), bno & 0xff);
  outb(clbam(c), (bno >> 8) & 0xff);
  outb(clbah(c), (bno >> 16) & 0xff);
  outb(cdevice(c), 0xe0 | 
                   (d->master ? 0 : 0x10) |
                   ((bno>>24) & 0x0f));
  if (write) {
    outb(cstatus(c), CMD_WRITE);
    outsw(cdata(c), buf, 512/2);
  } else {
    outb(cstatus(c), CMD_READ);
    sleep(&c->wait, &c->lk);
    insw(cdata(c), buf, 512/2);
  }
  unlock(&c->lk);
  return 0;
}

static void
idechaninit(int c)
{
  idechan *chan;
  idedisk *disk;
  block *blk;
  int ioirq, irqno;
  chan = kmalloc(sizeof *chan);
  if (!chan)
    panic("idechan");
  slockinit(&chan->lk);
  waitinit(&chan->wait);
  chan->reg = c == 0 ? 0x1f0 : 0x170;
  ioirq = c == 0 ? 14 : 15;
  irqno = ioapicen(ioirq);
  if (irqno < 0)
    panic("idechan");
  newirq(irqno, lapiceoi, ideintr, chan);
  for (int i = 0; i < 2; i++) {
    disk = &chan->disk[i];
    disk->chan = chan;
    disk->master = i == 0;
    printk("chan %d i %d\n", c, i);
    if (!diskpresent(disk))
      continue;
    blk = newblock(idebio, disk);
    if (!blk)
      panic("blk");
  }
  if (chan->disk[0].present)
    diskwait(&chan->disk[0]);
  if (chan->disk[1].present)
    diskwait(&chan->disk[1]);
}

void
ideinit(void)
{
  for (int i = 0; i < 2; i++) {
    idechaninit(i);
  }
}
