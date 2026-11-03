#include <kernel.h>
#include <sys.h>
#include <asm.h>
#include <syscall.h>
#include <proc.h>

#define ARG0  (tf->rdi)
#define ARG1  (tf->rsi)
#define ARG2  (tf->rdx)
#define ARG3  (tf->rcx)

static file*
fdfile(int fd)
{
  proc *p = myproc();
  if (fd < 16)
    return p->openf[fd];
  return NULL;
}

static ulong
sysnull(trapframe *tf)
{
  return (ulong)-1;
}

static ulong
sysexec(trapframe *tf)
{
  char name[128];
  if (!validstr(ARG0, 128, 0))
    return -1;
  strcpy(name, (const char*)ARG0);
  return (ulong)exec(name);
}

static ulong
sysread(trapframe *tf)
{
  file *f;
  f = fdfile(ARG0);
  if (!f)
    return -1;
  if (!validate(ARG1, ARG2, 1))
    return -1;
  return (ulong)read(f, (void*)ARG1, ARG2);
}

static ulong
syswrite(trapframe *tf)
{
  file *f;
  f = fdfile(ARG0);
  if (!f)
    return -1;
  if (!validate(ARG1, ARG2, 0))
    return -1;
  return (ulong)write(f, (void*)ARG1, ARG2);
}

static ulong
sysfork(trapframe *tf)
{
  return (ulong)fork();
}

static ulong
sysexit(trapframe *tf)
{
  int exitcode;
  exitcode = ARG0;
  return (ulong)exit(exitcode);
}

static ulong
syswait(trapframe *tf)
{
  if (ARG0 && !validate(ARG0, sizeof(int), 1))
    return -1;
  return wait((int*)ARG0);
}

static ulong
sysopen(trapframe *tf)
{
  char name[128];
  if (!validstr(ARG0, 128, 0))
    return -1;
  strcpy(name, (const char*)ARG0);
  return open(name, (int)ARG1);
}

static ulong
sysclose(trapframe *tf)
{
  return -1;
  // return close((int)ARG0);
}

static ulong (*syscalls[NR_SYSCALL])(trapframe *tf) = {
  sysnull,
  sysexec,
  sysread,
  syswrite,
  sysfork,
  sysexit,
  syswait,
  sysopen,
  sysclose,
};

void
syscall(trapframe *tf)
{
  ulong no = tf->rax;
  if (no < NR_SYSCALL && syscalls[no])
    tf->rax = syscalls[no](tf);
  else
    tf->rax = (ulong)-1;
}
