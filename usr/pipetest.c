#include "user.h"

#define N 128

int
main(void)
{
  int fds[2];
  int pid;
  int wpid;
  char wbuf[N];
  char rbuf[N];
  int r;
  int off;
  if (pipe(fds) < 0) {
    printf("pipe failed\n");
    exit(1);
  }
  for (int i = 0; i < N; i++)
    wbuf[i] = 'a' + (i % 26);
  pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    close(fds[1]);
    off = 0;
    while (off < N) {
      r = read(fds[0], rbuf + off, N - off);
      if (r <= 0) {
        printf("read short: off=%d r=%d\n", off, r);
        exit(1);
      }
      off += r;
    }
    close(fds[0]);
    for (int i = 0; i < N; i++) {
      if (rbuf[i] != wbuf[i]) {
        printf("mismatch at %d: got %d expected %d\n", i, rbuf[i], wbuf[i]);
        exit(1);
      }
    }
    printf("child ok %d bytes\n", N);
    exit(0);
  }
  close(fds[0]);
  off = 0;
  while (off < N) {
    r = write(fds[1], wbuf + off, N - off);
    if (r <= 0) {
      printf("write short: off=%d r=%d\n", off, r);
      exit(1);
    }
    off += r;
  }
  close(fds[1]);
  wait(&wpid);
  printf("parent done\n");
  exit(0);
}
