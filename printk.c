#include <kernel.h>
#include <printk.h>
#include <console.h>
#include "string.h"

typedef struct OPTION
{
  int     n;
  bool    zero;
  bool    pr0x;
  bool    strcut;
} OPTION;

static uint
sprintiu32 (char *p, i32 num, int base, bool sign, OPTION opt)
{
  char buf[sizeof (num) * 8 + 1] = {0};
  char *end = buf + sizeof (buf);
  char *cur = end - 1;
  bool neg = false;
  u32 unum;
  uint n = 0;
  uint ndigit = opt.n;

  if (sign && num < 0) {
    unum = (u32)(-(num + 1)) + 1;
    neg = true;
  } else {
    unum = (u32)num;
  }

  do {
    *--cur = "0123456789abcdef"[unum % base];
    n++;
  } while (unum /= base);

  if (neg) {
    *--cur = '-';
    n++;
  }

  while (ndigit > n) {
    *--cur = opt.zero ? '0' : ' ';
    n++;
  }

  memcpy (p, cur, n);
  return n;
}

static uint
sprintiu64 (char *p, i64 num, int base, bool sign, OPTION opt)
{
  char buf[sizeof (num) * 8 + 1] = {0};
  char *end = buf + sizeof (buf);
  char *cur = end - 1;
  bool neg = false;
  u64 unum;
  uint n = 0;
  uint ndigit = opt.n;

  if (sign && num < 0) {
    unum = (u64)(-(num + 1)) + 1;
    neg = true;
  } else {
    unum = (u64)num;
  }

  do {
    *--cur = "0123456789abcdef"[unum % base];
    n++;
  } while (unum /= base);

  if (neg) {
    *--cur = '-';
    n++;
  }

  while (ndigit > n) {
    *--cur = opt.zero ? '0' : ' ';
    n++;
  }

  memcpy(p, cur, n);
  return n;
}

static inline bool
isdigit (char c)
{
  return '0' <= c && c <= '9';
}

// %#x, %02x, %3d, etc...
static const char *
parseopt (const char *str, OPTION *opt)
{
  int n = 0;

  memset (opt, 0, sizeof *opt);

  if (*str == '.') {
    str++;
    opt->strcut = true;
  }
  if (*str == '#') {
    str++;
    opt->pr0x = true;
  }
  if (*str == '0') {
    str++;
    opt->zero = true;
  }

  while (isdigit (*str))
    n = n * 10 + *str++ - '0';

  opt->n = n;
  return str;
}

int
vsprintf (char *buf, const char *fmt, va_list ap)
{
  char    c;
  uint    n = 0;
  uint    len;
  OPTION  opt;

  for (; *fmt; fmt++) {
    c = *fmt;
    if (c == '%') {
      fmt = parseopt (++fmt, &opt);

      switch (*fmt) {
      case 'd':
  len = sprintiu32 (buf + n, va_arg (ap, i32), 10, true, opt);
  n += len;
  break;
      case 'u':
  len = sprintiu32 (buf + n, va_arg (ap, u32), 10, false, opt);
  n += len;
  break;
      case 'x':
  if (opt.pr0x)
  {
    buf[n++] = '0';
    buf[n++] = 'x';
  }
  len = sprintiu64 (buf + n, va_arg (ap, u64), 16, false, opt);
  n += len;
  break;
      case 'p':
  buf[n++] = '0';
  buf[n++] = 'x';
  len = sprintiu64 (buf + n, va_arg (ap, u64), 16, false, opt);
  n += len;
  break;
      case 'c': {
  int ch = va_arg (ap, int);
  buf[n++] = ch;
  break;
      }
      case 's': {
  char *s = va_arg (ap, char *);
  if (s == NULL)
    s = "(null)";

  len = strlen (s);
  len = (opt.strcut && len > opt.n) ? opt.n : len;
  memcpy (buf + n, s, len);
  n += len;
  break;
      }
      case '%':
  buf[n++] = '%';
  break;
      default:
  buf[n++] = '%';
  buf[n++] = c;
  break;
      }
    } else {
      buf[n++] = c;
    }
  }

  return n;
}

int
sprintf(char *buf, const char *fmt, ...)
{
  va_list ap;
  int n;

  va_start (ap, fmt);
  n = vsprintf (buf, fmt, ap);
  va_end (ap);
  return n;
}

int
printkcs(struct console *cs, const char *fmt, va_list ap)
{
  char buf[512] = {0};
  int n;

  n = vsprintf(buf, fmt, ap);
  cs->ops->write(cs, buf, n);
  return n;
}

int
printk(const char *fmt, ...)
{
  struct console *cs = console;
  va_list ap;
  int n;

  if (!cs)
    return -1;

  va_start (ap, fmt);
  n = printkcs (cs, fmt, ap);
  va_end(ap);

  return n;
}
