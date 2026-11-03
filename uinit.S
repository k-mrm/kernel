.text
main:
  movl  $1, %eax
  movq  $init, %rdi
  movq  $argv, %rsi
  int   $0x80
  movl  $5, %eax
  movl  $1, %edi
  int   $0x80
1:
  jmp   1b

init:
  .asciz "/init"
  .p2align 3
argv:
  .quad init
  .quad 0
