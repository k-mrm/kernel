#include "user.h"

static void
uputc(char c)
{
  write(1, &c, 1);
}

static int
printd(long long v, int base, bool sign, void (*putc)(char))
{
  char buf[sizeof(v) * 8 + 1] = {0};
  char *end = buf + sizeof buf;
  char *cur = end - 1;
  bool neg;
  unsigned long long num;
  char c;
  int n = 0;
  if (sign && v < 0) {
    num = -(unsigned long long)v;
    neg = true;
  } else {
    num = (unsigned long long)v;
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
__printf(const char *fmt, va_list ap, void (*putc)(char))
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
          n += printd(va_arg(ap, int), 10, true, putc);
          break;
        case 'p':
          (*putc)('0'); n++;
          (*putc)('x'); n++;
          __attribute__((fallthrough));
        case 'x':
          n += printd(va_arg(ap, unsigned long long), 16, false, putc);
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
printf(const char *fmt, ...)
{
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = __printf(fmt, ap, uputc);
  va_end(ap);
  return n;
}

void*
memcpy(void *dst, const void *src, ulong n)
{
  return memmove(dst, src, n);
}

void*
memmove(void *dst, const void *src, ulong n)
{
  char *d = dst;
  const char *s = src;
  if (s > d) {
    while (n-- > 0)
      *d++ = *s++;
  } else {
    d += n;
    s += n;
    while (n-- > 0)
      *--d = *--s;
  }
  return dst;
}

void*
memset(void *dst, int c, ulong n)
{
  char *d = dst;
  while (n-- > 0)
    *d++ = c;
  return dst;
}

char*
strcpy(char *dst, const char *src)
{
  char *r = dst;
  while ((*dst++ = *src++) != 0)
    ;
  return r;
}

int
strcmp(const char *s1, const char *s2)
{
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

int
memcmp(const void *a, const void *b, uint n)
{
  const uchar *s1 = a;
  const uchar *s2 = b;
  while (n > 0) {
    if (*s1 != *s2)
      return *s1 - *s2;
    s1++; s2++;
    n--;
  }
  return 0;
}

int
strncmp(const char *s1, const char *s2, ulong len)
{
  while (*s1 && *s1 == *s2 && len > 0) {
    s1++;
    s2++;
    len--;
  }
  if (len == 0)
    return 0;
  return *s1 - *s2;
}

uint
strlen(const char *s)
{
  uint i = 0;
  while (*s++)
    i++;
  return i;
}

char *
strchr(const char *s, int c)
{
  char *p = (char *)s;
  while (*p) {
    if (*p == c)
      return p;
    p++;
  }
  return NULL;
}

char *
strtok(char *s1, const char *s2)
{
  static char *save = "";
  char *s;
  if (!s1)
    s1 = save;
  while (*s1 && strchr(s2, *s1))
    s1++;
  if (!*s1)
    return NULL;
  s = s1;
  while (*s1 && !strchr(s2, *s1))
    s1++;
  if (*s1)
    *s1++ = 0;
  save = s1;
  return s;
}
