#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <memlayout.h>

void NORETURN
apmain(void)
{
  for (;;)
    hlt();
}

void NORETURN
bspmain(void)
{
  serial_init();
  seginit();
  trapinit();
  pageinit1((ulong)va(1024*1024*1024));  // 1GiB
  kernelmap();
  lapicinit();
  asm volatile ("sti");
  for (;;)
    hlt();
}
