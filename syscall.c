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

static int
fdsetfile(int fd, file *f)
{
  proc *p = myproc();
  if (fd < 16) {
    p->openf[fd] = f;
    return 0;
  }
  return -1;
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
  char *argv[16];
  char *uarg;
  if (!validstr(ARG0, 128, 0))
    return -1;
  strcpy(name, (const char*)ARG0);
  for (int i = 0; ; i++) {
    if (!validate(ARG1 + i*8, 8, 0))
      goto out;
    uarg = *(char**)(ARG1 + i*8);
    if (uarg == NULL) {
      argv[i] = NULL;
      break;
    }
    if (!validstr((ulong)uarg, 128, 0))
      goto out;
    argv[i] = kmalloc(strlen(uarg));
    strcpy(argv[i], uarg);
  }
  return (ulong)exec(name, argv);
out:
  return -1;
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
  file *f;
  f = fdfile(ARG0);
  if (!f)
    return -1;
  fdsetfile(ARG0, NULL);
  return (ulong)close(f);
}

static ulong
syspipe(trapframe *tf)
{
  int *fds = (int*)ARG0;
  if (!validate((ulong)fds, sizeof(int)*2, 1))
    return -1;
  return (ulong)newpipe(fds);
}

static ulong
sysdup2(trapframe *tf)
{
  file *f, *of;
  f = fdfile(ARG0);
  if (!f)
    return -1;
  of = fdfile(ARG1);
  if (fdsetfile(ARG1, fdup(f)) < 0)
    return -1;
  if (of)
    close(of);
  return ARG1;
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
  syspipe,
  sysdup2,
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
