#include "user.h"

static char line[1024];

static int
match(const char *pat, const char *s)
{
  uint plen = strlen(pat);
  for (; *s; s++) {
    if (strncmp(s, pat, plen) == 0)
      return 1;
  }
  return 0;
}

static void
grep(const char *pat, int fd)
{
  int n;
  int i = 0;
  char c;
  while ((n = read(fd, &c, 1)) > 0) {
    if (c == '\n') {
      line[i] = 0;
      if (match(pat, line))
        printf("%s\n", line);
      i = 0;
    } else if (i < (int)sizeof line - 1) {
      line[i++] = c;
    }
  }
  if (i > 0) {
    line[i] = 0;
    if (match(pat, line))
      printf("%s\n", line);
  }
}

int
main(int argc, char **argv)
{
  int fd;
  if (argc < 2) {
    printf("usage: grep pattern [file..]\n");
    exit(1);
  }
  if (argc < 3) {
    grep(argv[1], 0);
  } else {
    for (int i = 2; i < argc; i++) {
      if ((fd = open(argv[i], O_READ)) < 0) {
        printf("grep: cannot open %s\n", argv[i]);
        continue;
      }
      grep(argv[1], fd);
      close(fd);
    }
  }
  exit(0);
}
