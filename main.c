#include <kernel.h>
#include <asm.h>
#include <sys.h>

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
  for (;;)
    hlt();
}
