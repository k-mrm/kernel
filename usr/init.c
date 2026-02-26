#include <syscall.h>
  
int 
main (void)
{
  int pid;
  const char *args[1] = {"cat"};

  for (;;) {
    pid = fork();
    if (pid == 0) {
      exec("cat", args);
      write(1, "failed\n", 7);
    }

    wait(NULL);
  }
}
