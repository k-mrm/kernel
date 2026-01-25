#include <kernel.h>
#include <console.h>
#include <printk.h>

struct console *console = NULL;

int
probe_console(struct console *cs)
{
	console = cs;
	return 0;
}
