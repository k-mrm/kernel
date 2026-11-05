#include "user.h"

static char line[1024];

static void
revline(int fd)
{
  int n = 0;
  char c;
  int r;
  for (;;) {
    r = read(fd, &c, 1);
    if (r <= 0) {
      if (n > 0) {
        for (int i = n - 1; i >= 0; i--)
          write(1, &line[i], 1);
        write(1, "\n", 1);
      }
      return;
    }
    if (c == '\n') {
      for (int i = n - 1; i >= 0; i--)
        write(1, &line[i], 1);
      write(1, "\n", 1);
      n = 0;
    } else if (n < (int)sizeof line) {
      line[n++] = c;
    }
  }
}

int
main(int argc, char **argv)
{
  int fd;
  if (argc < 2) {
    revline(0);
  } else {
    for (int i = 1; i < argc; i++) {
      if ((fd = open(argv[i], O_READ)) < 0) {
        printf("rev: cannot open %s\n", argv[i]);
        continue;
      }
      revline(fd);
      close(fd);
    }
  }
  exit(0);
}
