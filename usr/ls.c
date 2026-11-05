#include "user.h"

struct udentry {
  char name[256];
  int inum;
  unsigned short mode;
};

static char*
format(unsigned short m, char c[])
{
  int perm = m & 0777;
  c[0] = S_ISDIR(m) ? 'd' : '-';
  for (int bit = 0; bit < 9; bit++) {
    c[9 - bit] = perm & (1 << bit) ? "xwr"[bit % 3] : '-';
  }
  return c;
}

int
ls(char *path)
{
  struct udentry ud = {0};
  char t[11] = {0};
  int fd = open(path, O_READ);
  if (fd < 0) {
    printf("No such file or directory\n");
    return -1;
  }
  while (read(fd, &ud, sizeof(ud)) > 0) {
    printf("%s %s\t%d\n", format(ud.mode, t), ud.name, ud.inum);
    memset(&ud, 0, sizeof ud);
  }
  close(fd);
  return 0;
}

int
main(int argc, char **argv)
{
  int err = 0;
  if (argc < 2) {
    ls(".");
  } else {
    for (int i = 1; i < argc; i++) {
      if (ls(argv[i]) < 0)
        err = 1;
    }
  }
  exit(err);
}
