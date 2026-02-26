#include <kernel.h>
#include <string.h>
#include <console.h>
#include <printk.h>
#include <proc.h>

#define C(x)  ((x)-'@')

struct console *console = NULL;

int
probe_console(struct console *cs)
{
  memset(&cs->buf, 0, sizeof cs->buf);
  cs->buf.cbuf = &cs->buf.buf[0];

  console = cs;
  return 0;
}

static struct cbuf *
flipped(struct flipbuf *fbuf)
{
  if (fbuf->cbuf == &fbuf->buf[0])
    return &fbuf->buf[1];
  else if (fbuf->cbuf == &fbuf->buf[1])
    return &fbuf->buf[0];
  return NULL;
}

static void
flip(struct flipbuf *fbuf)
{
  fbuf->cbuf = flipped(fbuf);
}

static int
fbuf_wr(struct flipbuf *fbuf, char c)
{
  struct cbuf *cbuf = fbuf->cbuf;

  cbuf->data[cbuf->write++ % 2048] = c;

  return 0;
}

static char
fbuf_rd(struct flipbuf *fbuf)
{
  struct cbuf *cbuf = flipped(fbuf);

  if (cbuf->read == cbuf->write) {
    return -1;
  }

  return cbuf->data[cbuf->read++ % 2048];
}

static int
coming(void *arg)
{
  struct flipbuf *fbuf = arg;
  struct cbuf *cbuf = flipped(fbuf);

  return cbuf->read != cbuf->write;
}

int
consread(char *buf, int size)
{
  struct console *cs = console;
  int n = size;
  char c;

  while (n > 0) {
    sleep(&cs->chan, coming, &cs->buf);

    c = fbuf_rd(&cs->buf);

    if (c == C('D'))
      break;
    *buf++ = c;
    n--;
    if (c == '\n')
      break;
  }

  return size - n;
}

int
console_irq(struct irq *irq)
{
  struct console *cs = dev_console(irq_device(irq));
  struct flipbuf *fbuf = &cs->buf;
  char c;

  while ((c = cs->ops->read(cs)) >= 0) {
    if (c == 0)
      continue;

    fbuf_wr(fbuf, c);
  }

  flip(fbuf);
  wakeup(&cs->chan);

  return 0;
}
