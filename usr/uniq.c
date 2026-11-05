#include "user.h"

static char cur[1024];
static char prev[1024];

static void
uniq(int fd)
{
  int n = 0;
  char c;
  int first = 1;
  while (read(fd, &c, 1) > 0) {
    if (c == '\n') {
      cur[n] = 0;
      if (first || strcmp(cur, prev) != 0) {
        printf("%s\n", cur);
        strcpy(prev, cur);
        first = 0;
      }
      n = 0;
    } else if (n < (int)sizeof cur - 1) {
      cur[n++] = c;
    }
  }
}

int
main(int argc, char **argv)
{
  int fd;
  if (argc < 2) {
    uniq(0);
  } else {
    if ((fd = open(argv[1], O_READ)) < 0) {
      printf("uniq: cannot open %s\n", argv[1]);
      exit(1);
    }
    uniq(fd);
    close(fd);
  }
  exit(0);
}
