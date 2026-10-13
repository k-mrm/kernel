#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <memlayout.h>

void NORETURN
kmain(void)
{
  loadidt();
  lapicinit();
  seginit();
  schedule();
  panic("exit");
}

void NORETURN
bspmain(void)
{
  serial_init();
  trapinit();
  cpu0init();
  pageinit1((ulong)va(1024*1024*1024));  // 1GiB
  kernelmap();
  kmallocinit();
  ioapicinit();
  ideinit();
  procinit();
  kmain();
}

void NORETURN
apmain(void)
{
  cpuinit();
}
