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
  exec(name);
  return 0;
}

static ulong
sysread(trapframe *tf)
{
  return (ulong)-1;
}

static ulong
syswrite(trapframe *tf)
{
  printk("syswrite enter\n");
  panic("wr %s\n", (const char*)ARG1);
  return (ulong)-1;
}

static ulong
sysfork(trapframe *tf)
{
  return (ulong)-1;
}

static ulong (*syscalls[NR_SYSCALL])(trapframe *tf) = {
  sysnull,
  sysexec,
  sysread,
  syswrite,
  sysfork,
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
