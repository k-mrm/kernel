#ifndef _USER_H
#define _USER_H

#define SYS_NULL    0
#define SYS_EXEC    1
#define SYS_READ    2
#define SYS_WRITE   3
#define SYS_FORK    4

#ifndef __ASSEMBLER__

int exec(const char*, const char**);
int write(int, void*, int);
int read(int, void*, int);
int fork(void);

#endif  // __ASSEMBLER__

#endif
