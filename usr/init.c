#include <syscall.h>
	
int 
main (void)
{
	int pid;
	const char *args[3] = {"echo", "hello echo!", "hello!"};

	for (;;) {
		pid = fork();
		if (pid == 0) {
			exec("echo", args);
			write(1, "failed\n", 7);
		}

		wait(NULL);
	}
}
