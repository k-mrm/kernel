#include "user.h"
  
int
main(void)
{
  const char *argv[2] = {"sh", 0};
  int pid, wpid;
  open("/dev/cons", O_READ);  // 0
  open("/dev/cons", O_WRITE); // 1
  open("/dev/cons", O_WRITE); // 2
  for (;;) {
    printf("sh\n");
    pid = fork();
    if (pid < 0) {
      printf("init: fork failed\n");
      exit(-1);
    }
    if (pid == 0) {
      printf("child process\n");
      exec("sh", argv);
      exit(-1);
    }
    printf("parent process %d\n", pid);
    wait(&wpid);
    printf("wait %d\n", wpid);
  }
}

