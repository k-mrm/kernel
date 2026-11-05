#include "user.h"

int
main(void)
{
  int p2c[2];
  int c2p[2];
  int pid;
  int wpid;
  char buf[8];
  if (pipe(p2c) < 0 || pipe(c2p) < 0) {
    printf("pipe failed\n");
    exit(1);
  }
  pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    close(p2c[1]);
    close(c2p[0]);
    if (read(p2c[0], buf, 4) != 4) {
      printf("child: read failed\n");
      exit(1);
    }
    buf[4] = 0;
    printf("child got %s\n", buf);
    if (write(c2p[1], "pong", 4) != 4) {
      printf("child: write failed\n");
      exit(1);
    }
    close(p2c[0]);
    close(c2p[1]);
    exit(0);
  }
  close(p2c[0]);
  close(c2p[1]);
  if (write(p2c[1], "ping", 4) != 4) {
    printf("parent: write failed\n");
    exit(1);
  }
  if (read(c2p[0], buf, 4) != 4) {
    printf("parent: read failed\n");
    exit(1);
  }
  buf[4] = 0;
  printf("parent got %s\n", buf);
  close(p2c[1]);
  close(c2p[0]);
  wait(&wpid);
  exit(0);
}
