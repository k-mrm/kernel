#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <memlayout.h>

void NORETURN
kmain(void)
{
  loadidt();
  lapicinit();
  cpuinit();
  seginit();
  schedule();
  panic("exit");
}

void NORETURN
bspmain(void)
{
  serial_init();
  trapinit();
  pageinit1((ulong)va(1024*1024*1024));  // 1GiB
  kernelmap();
  kmallocinit();
  procinit();
  kmain();
}

void NORETURN
apmain(void)
{
  ;
}
