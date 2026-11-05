#include "user.h"

static char buf[512];

static void
nl(int fd)
{
  int n;
  int line = 1;
  int head = 1;
  while ((n = read(fd, buf, sizeof buf)) > 0) {
    for (int i = 0; i < n; i++) {
      if (head) {
        printf("%d\t", line);
        head = 0;
      }
      write(1, &buf[i], 1);
      if (buf[i] == '\n') {
        line++;
        head = 1;
      }
    }
  }
}

int
main(int argc, char **argv)
{
  int fd;
  if (argc < 2) {
    nl(0);
  } else {
    for (int i = 1; i < argc; i++) {
      if ((fd = open(argv[i], O_READ)) < 0) {
        printf("nl: cannot open %s\n", argv[i]);
        continue;
      }
      nl(fd);
      close(fd);
    }
  }
  exit(0);
}
