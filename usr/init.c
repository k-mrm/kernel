#include "user.h"
  
int
main(void)
{
  write(1, "userhello\n", 10);

  for (;;)
    ;
}
