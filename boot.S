#include <asm.h>

.code16
start:
  cli
  xorw  %ax, %ax
  movw  %ax, %ds 
  movw  %ax, %es 
  movw  %ax, %ss 
  // A20
1:
  inb   $0x64, %al
  testb $0x2, %al
  jnz   1b
  movb  $0xd1, %al
  outb  %al, $0x64
2:
  inb   $0x64, %al
  testb $0x2, %al
  jnz   2b
  movb  $0xdf, %al
  outb  %al, $0x60
  // setup sp
  movl  $start, %esp
  // load kernel
  movb  $0x80, %dl
  movl  $0x1, %ebx
  movw  $0x2000, %ax
  movw  %ax, %es  // addr=0x20000
loadboot:
  call  rdsect
  incl  %ebx
  addw  $0x20, %ax
  movw  %ax, %es    // addr += 512
  cmpl  $512, %ebx  // LBA <= 512
  jbe   loadboot
  
  ljmp  $0x2000, $0x0000

  // DL: drive
  // ebx: LBA
  // es:0000: addr
rdsect:
  pusha 
  xorl  %eax, %eax
  pushl %eax
  pushl %ebx 
  pushw %es
  pushw $0
  pushw $1
  pushw $16
  movb  $0x42, %ah
  movw  %sp, %si
  int   $0x13
  popa
  popa
  ret

  .org  0x1fe
  .word 0xaa55
