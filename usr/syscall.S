#include "user.h"

.text
.global exec
exec:
  movl $SYS_EXEC, %eax
  int $0x80
  ret
.global read
read:
  movl $SYS_READ, %eax
  int $0x80
  ret
.global write
write:
  movl $SYS_WRITE, %eax
  int $0x80
  ret
.global fork
fork:
  movl $SYS_FORK, %eax
  int $0x80
  ret
