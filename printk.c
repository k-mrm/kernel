#include <kernel.h>
#include <sys.h>
#include <asm.h>

static void
pputc(char c, char *_)
{
  serial_putc(c);
}

static void
sputc(char c, char *p)
{
  *p = c;
}

static int
printd(i64 v, int base, bool sign, char *buf, void (*putc)(char, char*))
{
  char tmp[sizeof(v) * 8 + 1] = {0};
  char *end = tmp + sizeof tmp;
  char *cur = end - 1;
  bool neg;
  u64 num;
  char c;
  int n = 0;
  if (sign && v < 0) {
    num = -(u64)v;
    neg = true;
  } else {
    num = (u64)v;
    neg = false;
  }
  do {
    *--cur = "0123456789abcdef"[num % base];
  } while (num /= base);
  if (neg)
    *--cur = '-';
  for (; (c = *cur) != 0; cur++, n++)
    (*putc)(c, buf+n);
  return n;
}

static int
__printk(const char *fmt, va_list ap, char *buf, void (*putc)(char, char*))
{
  int i, n = 0;
  char c, *s, cs;
  if (!fmt)
    return 0;
  for (i = 0; (c = fmt[i]) != 0; i++) {
    if (c == '%') {
      c = fmt[++i];
      if (!c)
        break;
      switch (c) {
        case 'd':
          n += printd(va_arg(ap, i32), 10, true, buf+n, putc);
          break;
        case 'p':
          (*putc)('0', buf+n); n++;
          (*putc)('x', buf+n); n++;
          FALLTHROUGH;
        case 'x':
          n += printd(va_arg(ap, u64), 16, false, buf+n, putc);
          break;
        case 'c':
          (*putc)(va_arg(ap, int), buf+n); n++;
          break;
        case 's':
          s = va_arg(ap, char*);
          if (!s)
            s = "(null)";
          for (; (cs = *s) != 0; s++, n++)
            (*putc)(cs, buf+n);
          break;
        case '%':
          (*putc)('%', buf+n); n++;
          break;
        default:
          (*putc)('%', buf+n); n++;
          (*putc)(c, buf+n); n++;
          break;
      }
    } else {
      (*putc)(c, buf+n); n++;
    }
  }
  return n;
}

int
sprintf(char *buf, const char *fmt, ...)
{
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = __printk(fmt, ap, buf, sputc);
  va_end(ap);
  return n;
}

int
printk(const char *fmt, ...)
{
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = __printk(fmt, ap, NULL, pputc);
  va_end(ap);
  return n;
}

void
backtrace(ulong rbp)
{
  ulong *frame;
  ulong ret, prev;
  printk("backtrace:\n");
  for (int i = 0; i < 20; i++) {
    if (rbp < 0xffff800000000000UL)
      break;
    frame = (ulong*)rbp;
    prev = frame[0];
    ret = frame[1];
    printk("  [%d] rip=%p rbp=%p\n", i, ret, rbp);
    if (prev <= rbp)
      break;
    rbp = prev;
  }
}

void NORETURN
panic(const char *fmt, ...)
{
  va_list ap;
  cli();
  printk("panic: ");
  va_start(ap, fmt);
  __printk(fmt, ap, NULL, pputc);
  va_end(ap);
  printk("\n");
  for (;;)
    ;
}
