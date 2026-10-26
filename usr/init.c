#include "user.h"
  
int
main(void)
{
  const char *argv[2] = {"sh", 0};
  int pid, wpid;
  for (;;) {
    printf("init: starting sh\n");
    pid = fork();
    if (pid < 0) {
      printf("init: fork failed\n");
      exit(-1);
    }
    if (pid == 0) {
      exec("sh", argv);
      exit(-1);
    }
    wait(NULL);
  }
}

