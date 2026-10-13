#include <kernel.h>
#include <sys.h>
#include <wait.h>

void
waitinit(waitpoint *w)
{
  slockinit(&w->lk);
  w->p = NULL;
}
