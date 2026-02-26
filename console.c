#include <console.h>
#include <kernel.h>
#include <printk.h>
#include <proc.h>
#include <string.h>

#define C(x) ((x) - '@')

struct console *console = NULL;

int probe_console(struct console *cs) {
  memset(&cs->buf, 0, sizeof cs->buf);
  cs->buf.cbuf = &cs->buf.buf[0];
  console = cs;
  return 0;
}

static int fbuf_wr(struct flipbuf *fbuf, char c) {
  struct cbuf *cbuf = fbuf->cbuf;
  cbuf->data[cbuf->write++ % 2048] = c;
  return 0;
}

static void fbufbs(struct flipbuf *fbuf) {
  struct cbuf *cbuf = fbuf->cbuf;
  if (cbuf->read != cbuf->write)
    cbuf->write--;
}

static char fbuf_rd(struct flipbuf *fbuf) {
  struct cbuf *cbuf = fbuf->cbuf;
  if (cbuf->read == cbuf->write)
    return -1;
  return cbuf->data[cbuf->read++ % 2048];
}

static int coming(void *arg) {
  struct flipbuf *fbuf = arg;
  struct cbuf *cbuf = fbuf->cbuf;
  return cbuf->read != cbuf->write;
}

int consread(char *buf, int size) {
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

int console_irq(struct irq *irq) {
  struct console *cs = dev_console(irq_device(irq));
  struct flipbuf *fbuf = &cs->buf;
  char c;
  int w = 0;

  cs->ops->write(cs, " ", 0);
  while ((c = cs->ops->read(cs)) >= 0) {
    if (c == 0)
      continue;
    c = (c == '\r') ? '\n' : c;
    if (c == '\x7f') {
      fbufbs(fbuf);
      cs->ops->write(cs, "\b \b", 3); // echoback
    } else {
      if (c == '\n' || c == C('D'))
        w = 1;
      fbuf_wr(fbuf, c);
      cs->ops->putc(cs, c);   // echoback
    }
  }
  if (w)
    wakeup(&cs->chan);
  return 0;
}
