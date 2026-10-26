#include <kernel.h>
#include <sys.h>
#include <asm.h>
#include <syscall.h>

#define ARG0  (tf->rdi)
#define ARG1  (tf->rsi)
#define ARG2  (tf->rdx)
#define ARG3  (tf->rcx)

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
  if (!validate(ARG1, ARG2, 0))
    return -1;
  return (ulong)-1;
}

static ulong
syswrite(trapframe *tf)
{
  uint i;
  if (!validate(ARG1, ARG2, 0))
    return -1;
  // stub
  for (i = 0; i < ARG2; i++) {
    printk("%c", ((char*)ARG1)[i]);
  }
  return i;
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
  return exit(exitcode);
}

static ulong
syswait(trapframe *tf)
{
  if (ARG0 && !validate(ARG0, sizeof(int), 1))
    return -1;
  return wait((int*)ARG0);
}

static ulong (*syscalls[NR_SYSCALL])(trapframe *tf) = {
  sysnull,
  sysexec,
  sysread,
  syswrite,
  sysfork,
  sysexit,
  syswait,
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
