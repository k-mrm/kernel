#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <kernel.h>

typedef struct SYSCALL
{
  int sysno;
  void *syscall;
} SYSCALL;

#define SYSCALL_DEFINE(_no, _name)        \
  static USED SECTION (".data.syscall") ALIGNED (_Alignof (SYSCALL))      \
  SYSCALL __syscall_ ## _name = {                                         \
    .sysno = _no,                                                   \
    .syscall = (void *)_name,                                       \
  }

#define SYS_EMPTY       0
#define SYS_EXEC        1
#define SYS_READ        2
#define SYS_WRITE       3
#define SYS_FORK        4
#define SYS_WAIT        5
#define SYS_EXIT        6

void syscallinit (void);
void *syscall(int sysno, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6);

#endif  // _SYSCALL_H
