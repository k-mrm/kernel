#include "user.h"

int
main(int argc, char **argv)
{
  char *s;
  int len;
  if (argc < 2)
    s = "y\n";
  else
    s = argv[1];
  len = strlen(s);
  for (;;) {
    if (write(1, s, len) != len)
      exit(1);
    if (argc >= 2)
      write(1, "\n", 1);
  }
}
