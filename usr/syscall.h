#ifndef _SYSCALL_H
#define _SYSCALL_H

typedef unsigned long size_t;

int exec(const char *path, const char **argv);
int read(int fd, char *buf, size_t size);
int write(int fd, const char *buf, size_t size);
int fork(void);
int wait(int *ret);

#endif
