#ifndef _PRINTK_H
#define _PRINTK_H

#include <kernel.h>

int printk(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list ap);

#ifndef KPREFIX
#define KPREFIX
#endif // KPREFIX

#define trace(fmt, ...) printk(KPREFIX " [trace] " fmt, ##__VA_ARGS__)
#define warn(fmt, ...) printk(KPREFIX " [warning] " fmt, ##__VA_ARGS__)
#define log(fmt, ...) printk(KPREFIX " " fmt, ##__VA_ARGS__)

#endif // _PRINTK_H
