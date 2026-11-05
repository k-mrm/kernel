#include "user.h"

int
main(int argc, char **argv)
{
  int pid, wpid;
  int n = 4;
  if (argc >= 2)
    n = argv[1][0] - '0';
  for (int i = 0; i < n; i++) {
    pid = fork();
    if (pid < 0) {
      printf("fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      printf("child %d\n", i);
      exit(i);
    }
    printf("spawned %d\n", pid);
  }
  for (int i = 0; i < n; i++) {
    wait(&wpid);
    printf("reaped %d\n", wpid);
  }
  exit(0);
}
