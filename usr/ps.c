#include "user.h"

struct udentry {
  char name[256];
  int inum;
  unsigned short mode;
};

static int
isnum(const char *s)
{
  if (!*s) return 0;
  while (*s) {
    if (*s < '0' || *s > '9') return 0;
    s++;
  }
  return 1;
}

int
main(int argc, char **argv)
{
  struct udentry ud = {0};
  char path[64];
  char buf[128];
  int fd, dirfd, n, pl;

  dirfd = open("/proc", O_READ);
  if (dirfd < 0) {
    printf("ps: cannot open /proc\n");
    exit(1);
  }
  printf("PID PPID STATE NAME\n");
  while (read(dirfd, &ud, sizeof(ud)) > 0) {
    if (isnum(ud.name)) {
      strcpy(path, "/proc/");
      pl = 6;
      strcpy(path + pl, ud.name);
      while (path[pl]) pl++;
      strcpy(path + pl, "/stat");
      fd = open(path, O_READ);
      if (fd >= 0) {
        n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
          buf[n] = 0;
          printf("%s\n", buf);
        }
        close(fd);
      }
    }
    memset(&ud, 0, sizeof ud);
  }
  close(dirfd);
  exit(0);
}
