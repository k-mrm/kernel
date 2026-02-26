#include <syscall.h>
#include "lib.h"

void *memset(void *dst, int c, u64 n) {
  char *d = dst;
  while(n-- > 0)
    *d++ = c;
  return dst;
}

char *strchr(const char *s, int c) {
  char *p = (char *)s;
  while(*p) {
    if(*p == c)
      return p;
    p++;
  }
  return NULL;
}

char *strtok(char *s1, const char *s2) {
  static char *save = "";
  if(s1)
    save = s1;
  else
    s1 = save;
  char *s = s1;
  while(*s1) {
    if(strchr(s2, *s1)) {
      *s1++ = 0;
      save = s1;
      return s;
    }
    s1++;
  }
  return NULL;
}

u64 strlen(const char *s) {
  u64 i = 0;
  while(*s++)
    i++;
  return i;
}

int strcmp(const char *s1, const char *s2) {
  while(*s1) {
    if(*s1 != *s2)
      break;
    s1++;
    s2++;
  }
  return *s1 > *s2? 1 : *s1 == *s2? 0 : -1;
}

static void uprintiu32(i32 num, int base, bool sign) {
  char buf[sizeof(num) * 8 + 1] = {0};
  char *end = buf + sizeof(buf);
  char *cur = end - 1;
  u32 unum;
  bool neg = false;

  if(sign && num < 0) {
    unum = (u32)(-(num + 1)) + 1;
    neg = true;
  } else {
    unum = (u32)num;
  }

  do {
    *--cur = "0123456789abcdef"[unum % base];
  } while(unum /= base);

  if(neg)
    *--cur = '-';

  write(1, cur, strlen(cur));
}

static void uprintiu64(i64 num, int base, bool sign) {
  char buf[sizeof(num) * 8 + 1] = {0};
  char *end = buf + sizeof(buf);
  char *cur = end - 1;
  u64 unum;
  bool neg = false;
  if(sign && num < 0) {
    unum = (u64)(-(num + 1)) + 1;
    neg = true;
  } else {
    unum = (u64)num;
  }
  do {
    *--cur = "0123456789abcdef"[unum % base];
  } while(unum /= base);
  if(neg)
    *--cur = '-';
  write(1, cur, strlen(cur));
}

int puts(char *s) {
  int r = write(1, s, strlen(s));
  write(1, "\n", 1);
  return r;
}

int printf(const char *fmt, ...) {
  char tmpc;
  char *tmps;
  va_list ap;
  va_start(ap, fmt);

  for(int i = 0; fmt[i]; i++) {
    char c = fmt[i];
    if(c == '%') {
      c = fmt[++i];
      switch(c) {
        case 'd':
          uprintiu32(va_arg(ap, i32), 10, true);
          break;
        case 'u':
          uprintiu32(va_arg(ap, u32), 10, false);
          break;
        case 'x':
          uprintiu64(va_arg(ap, u64), 16, false);
          break;
        case 'p':
          write(1, "0x", 2);
          uprintiu64(va_arg(ap, u64), 16, false);
          break;
        case 'c':
          tmpc = va_arg(ap, int);
          write(1, &tmpc, 1);
          break;
        case 's':
          tmps = va_arg(ap, char *);
          if(tmps == NULL)
            tmps = "(null)";
          write(1, tmps, strlen(tmps));
          break;
        case '%':
          write(1, &c, 1);
          break;
        default:
          write(1, "%", 1);
          write(1, &c, 1);
          break;
      }
    } else {
      write(1, &c, 1);
    }
  }
  va_end(ap);
  return 0;
}

int parse(char *cmd, char **argv) {
  static const char *ws = " \n\t";
  char *tok;
  int argc = 0;
  while(strchr(ws, *cmd))
    cmd++;
  argv[argc++] = strtok(cmd, ws);
  while((tok = strtok(NULL, ws)))
    argv[argc++] = tok;
  argv[argc] = 0;
  if(argv[0] == NULL)
    return -1;
  return argc;
}

void readcmd(char *buf) {
  memset(buf, 0, 128);
  write(1, "$ ", 2);
  read(0, buf, 128);
}

int builtincmd(int argc, char **argv) {
  if(strcmp(argv[0], "cd") == 0) {
    /*
    if(argc < 2) {
      if(chdir("/") < 0)
        return -1;
    } else if(argc == 2) {
      if(chdir(argv[1]) < 0) {
        puts("cd: failed");
        return -1;
      }
    } else {
      puts("too many args");
      return -1;
    }
    */
    return 1;
  } else if(strcmp(argv[0], "exit") == 0) {
    exit(0);
    return 1;
  }
  return 0;
}

int main(void) {
  char *argv[8];
  int argc;
  char buf[128] = {0};
  for(;;) {
    readcmd(buf);
    argc = parse(buf, argv);
    if(argc < 0)
      continue;
    if(builtincmd(argc, argv))
      continue;
    int pid = fork();
    if(pid == 0) {
      exec(argv[0], argv);
      printf("%s failed\n", argv[0]);
      exit(1);
    } else {
      int status;
      wait(&status);
    }
  }
}
