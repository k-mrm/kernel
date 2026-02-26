#include <syscall.h>

void cat(int fd) {
  char c;

  while (read(fd, &c, 1) == 1)
    write(1, &c, 1);
}

void main(int argc, char **argv) {
  if (argc == 1) {
    cat(0);
  } else {
    // TODO
  }

  exit(0);
}
