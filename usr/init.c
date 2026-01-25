#include <syscall.h>
	
int 
main (void)
{
	int pid;
	const char *args[3] = {"echotest", "hello echo!", "SuperHello"};

	for (;;) {
		pid = fork();
		if (pid == 0) {
			exec("echo", args);
			write(1, "failed\n", 7);
		}

		wait(NULL);
	}
}
