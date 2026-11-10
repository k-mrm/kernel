#include "user.h"

int
main(int argc, char **argv)
{
  volatile int *p = (int*)0xdeadbeef;
  *p = 42;
  return 0;
}
