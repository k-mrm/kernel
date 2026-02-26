#ifndef _PANIC_H
#define _PANIC_H

#include <kernel.h>

void panic(char *msg, ...) NORETURN;

#endif // _PANIC_H
