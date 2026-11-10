#include "user.h"

static uint
uatoi(const char *s)
{
  uint n = 0;
  while (*s >= '0' && *s <= '9')
    n = n * 10 + (*s++ - '0');
  return n;
}

static int
kill(const char *pid, int sig)
{
  char path[64];
  char sigstr[16];
  int fd, n;
  sprintf(path, "/proc/%s/signal", pid);
  n = sprintf(sigstr, "%d", sig);
  fd = open(path, O_WRITE);
  if (fd < 0) {
    printf("kill: %s: no such process\n", pid);
    return -1;
  }
  if (write(fd, sigstr, n) < 0) {
    printf("kill: %s: write failed\n", pid);
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

int
main(int argc, char **argv)
{
  int sig = 1;  // SIGKILL
  int arg = 1;
  int err = 0;
  if (argc < 2) {
    printf("usage: kill [-N] PID...\n");
    exit(1);
  }
  if (argv[1][0] == '-') {
    sig = uatoi(&argv[1][1]);
    arg = 2;
  }
  if (arg >= argc) {
    printf("kill: missing PID\n");
    exit(1);
  }
  for (int i = arg; i < argc; i++) {
    if (kill(argv[i], sig) < 0)
      err = 1;
  }
  exit(err);
}
