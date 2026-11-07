#include <kernel.h>
#include <sys.h>
#include <asm.h>
#include <proc.h>
#include <fs.h>
#include <devfs.h>

typedef struct console console;

#define C(x)  ((x) - '@')

struct console {
  spinlock lk;
  waitpoint rwait;
  char buf[128];
  uint r, w, e;
};

static console *cons;

int
consread(devinode *di, void *buf, uint off, uint n)
{
  char c;
  char *cbuf = buf;
  uint t = n;
  console *cons = di->data;
  lock(&cons->lk);
  while (n > 0){
    while (cons->r == cons->w)
      sleep(&cons->rwait, &cons->lk);
    c = cons->buf[cons->r++ % 128];
    if (c == C('D'))
      break;
    *cbuf++ = c;
    n--;
    if (c == '\n')
      break;
  }
  unlock(&cons->lk);
  return t - n;
}

int
conswrite(devinode *di, void *buf, uint off, uint n)
{
  console *cons = di->data;
  lock(&cons->lk);
  for (uint i = 0; i < n; i++)
    serial_putc(((char*)buf)[i]);
  unlock(&cons->lk);
  return n;
}

void
consintr(char c)
{
  if (c == 0)
    return;
  lock(&cons->lk);
  switch (c) {
    case '\x7f':
      if (cons->e != cons->w) {
        cons->e--;
        serial_putc('\b');
        serial_putc(' ');
        serial_putc('\b');
      }
      break;
    default:
      if (cons->e - cons->r < 128){
        c = (c == '\r') ? '\n' : c;
        cons->buf[cons->e++ % 128] = c;
        serial_putc(c); // echoback
        if (c == '\n' || c == C('D') || cons->e == cons->r + 128) {
          cons->w = cons->e;
          wakeup(&cons->rwait);
        }
      }
      break;
  }
  unlock(&cons->lk);
}

int
devconsinit(devinode *di)
{
  console *c = kmalloc(sizeof *c);
  slockinit(&c->lk);
  waitinit(&c->rwait);
  di->data = c;
  di->read = consread;
  di->write = conswrite;
  cons = c;
  return 0;
}
