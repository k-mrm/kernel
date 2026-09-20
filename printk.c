#include <kernel.h>
#include <sys.h>

static int
printd(i64 v, int base, bool sign, void (*putc)(char))
{
  char buf[sizeof(v) * 8 + 1] = {0};
  char *end = buf + sizeof buf;
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
    (*putc)(c);
  return n;
}

static int
__printk(const char *fmt, va_list ap, void (*putc)(char))
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
          n += printd(va_arg(ap, i32), 10, true, putc);
          break;
        case 'p':
          (*putc)('0'); n++;
          (*putc)('x'); n++;
          FALLTHROUGH;
        case 'x':
          n += printd(va_arg(ap, u64), 16, false, putc);
          break;
        case 'c':
          (*putc)(va_arg(ap, int)); n++;
          break;
        case 's':
          s = va_arg(ap, char*);
          if (!s)
            s = "(null)";
          for (; (cs = *s) != 0; s++, n++)
            (*putc)(cs);
          break;
        case '%':
          (*putc)('%'); n++;
          break;
        default:
          (*putc)('%'); n++;
          (*putc)(c); n++;
          break;
      }
    } else {
      (*putc)(c); n++;
    }
  }
  return n;
}

int
printk(const char *fmt, ...)
{
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = __printk(fmt, ap, serial_putc);
  va_end(ap);
  return n;
}

void NORETURN
panic(const char *fmt, ...)
{
  va_list ap;
  asm volatile ("cli");
  printk("panic: ");
  va_start(ap, fmt);
  __printk(fmt, ap, serial_putc);
  va_end(ap);
  printk("\n");
  for (;;)
    ;
}
