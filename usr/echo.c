#include <syscall.h>

unsigned int strlen(const char *s) {
  unsigned int i = 0;
  while (*s++)
    i++;
  return i;
}

void main(int argc, char **argv) {
  for (int i = 1; i < argc; i++)
    write(1, argv[i], strlen(argv[i]));

  write(1, "\n", 1);
  exit(0);
}
