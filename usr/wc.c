#include "user.h"

static char buf[512];

static void
count(int fd, int *lines, int *words, int *bytes)
{
  int n;
  int inword = 0;
  char c;
  while ((n = read(fd, buf, sizeof buf)) > 0) {
    for (int i = 0; i < n; i++) {
      c = buf[i];
      (*bytes)++;
      if (c == '\n')
        (*lines)++;
      if (c == ' ' || c == '\t' || c == '\n') {
        inword = 0;
      } else if (!inword) {
        inword = 1;
        (*words)++;
      }
    }
  }
}

static void
pr(int l, int w, int b, char *name)
{
  printf(" %d %d %d", l, w, b);
  if (name) {
    write(1, " ", 1);
    write(1, name, strlen(name));
  }
  write(1, "\n", 1);
}

int
main(int argc, char **argv)
{
  int l = 0, w = 0, b = 0;
  int fd;
  if (argc < 2) {
    count(0, &l, &w, &b);
    pr(l, w, b, 0);
  } else {
    for (int i = 1; i < argc; i++) {
      if ((fd = open(argv[i], O_READ)) < 0) {
        printf("wc: cannot open %s\n", argv[i]);
        continue;
      }
      l = w = b = 0;
      count(fd, &l, &w, &b);
      pr(l, w, b, argv[i]);
      close(fd);
    }
  }
  exit(0);
}
