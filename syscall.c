#include <kernel.h>
#include <sys.h>
#include <asm.h>
#include <syscall.h>

static ulong
sysnull(trapframe *tf)
{
  return (ulong)-1;
}

static ulong
sysexec(trapframe *tf)
{
  char *buf = kmalloc(512);
  bread(getblock("hdb"), 1, buf);
  for (int i = 0; i < 32; i++)
    printk("%d ", buf[i]);
  panic("f");
}

static ulong
sysread(trapframe *tf)
{
  return (ulong)-1;
}

static ulong
syswrite(trapframe *tf)
{
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
