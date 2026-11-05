#include "user.h"

static char buf[512];

static void
head(int fd, int n)
{
  int r;
  int lines = 0;
  while (lines < n && (r = read(fd, buf, sizeof buf)) > 0) {
    for (int i = 0; i < r; i++) {
      write(1, &buf[i], 1);
      if (buf[i] == '\n') {
        lines++;
        if (lines >= n)
          return;
      }
    }
  }
}

static int
atoi(const char *s)
{
  int n = 0;
  while (*s >= '0' && *s <= '9') {
    n = n * 10 + (*s - '0');
    s++;
  }
  return n;
}

int
main(int argc, char **argv)
{
  int n = 10;
  int fd;
  int argi = 1;
  if (argc >= 3 && strcmp(argv[1], "-n") == 0) {
    n = atoi(argv[2]);
    argi = 3;
  }
  if (argi >= argc) {
    head(0, n);
  } else {
    for (int i = argi; i < argc; i++) {
      if ((fd = open(argv[i], O_READ)) < 0) {
        printf("head: cannot open %s\n", argv[i]);
        continue;
      }
      head(fd, n);
      close(fd);
    }
  }
  exit(0);
}
