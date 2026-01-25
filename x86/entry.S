#include "arch.h"
#include "mm.h"
#include "seg.h"

#define MULTIBOOT2_MAGIC      0xe85250d6
#define MULTIBOOT2_FLAGS      0   // 32bit i386
#define MULTIBOOT2_LENGTH     (header_end - multiboot_header)
#define CHECKSUM              0x100000000 - (MULTIBOOT2_MAGIC + MULTIBOOT2_FLAGS + MULTIBOOT2_LENGTH)

#define pa(x)       (x - PAGE_OFFSET)
#define va(x)       (x + PAGE_OFFSET)

.p2align 2

.section ".text.head"

multiboot_header:
  .long MULTIBOOT2_MAGIC
  .long MULTIBOOT2_FLAGS
  .long MULTIBOOT2_LENGTH
  .long CHECKSUM

  .word 0
  .word 0
  .long 8
header_end:

.section ".text.boot"
.code32
.global entry32
entry32:
  cli

  // save multiboot information
  leal  pa(multiboot_magic), %edx
  movl  %eax, (%edx)
  leal  pa(multiboot_info), %edx
  movl  %ebx, (%edx)
  // init sp
  movl  $0x7c00, %eax
  movl  %eax, %esp

  call  bootcheck
  call  cpuverify

  // enable PAE
  movl  %cr4, %eax
  orl   $CR4_PAE, %eax
  movl  %eax, %cr4
  // init pgt
  call  initpgt
  // enable long mode
  movl  $IA32_EFER, %ecx
  rdmsr
  orl   $IA32_EFER_LME, %eax
  wrmsr

  lgdtl pa(entrygdt_p)

  // enable paging
  mov   %cr0, %eax
  orl   $(CR0_PE | CR0_PG), %eax
  mov   %eax, %cr0

  movl  $0x2f4b2f4f, (0xb8000)
  ljmpl $(SEG_KCODE << 3), $pa_trampoline64

  hlt

bootcheck:
  leal  pa(multiboot_magic), %edx
  movl  (%edx), %eax
  cmpl  $0x36d76289, %eax   // multiboot magic
  jne   booterror
  ret

cpuverify:
  // cpuid
  pushfl
  popl  %eax
  movl  %eax, %ecx
  xorl  $(1 << 21), %eax 
  pushl %eax
  popfl
  pushfl
  popl  %eax
  pushl %ecx
  popfl
  xorl  %eax, %ecx
  jz    nocpuid
  // longmode support
  movl  $0x80000001, %eax
  cpuid
  test  $CPUID_EXT1_EDX_64BIT, %edx
  jz    nolongmode
  ret

initpgt:
  cld
  xorl  %eax, %eax
  leal  pa(__boot_pgt_s), %ebx  // __boot_pml4
  movl  %ebx, %edi
  leal  pa(__boot_pgt_e), %ecx
  subl  %edi, %ecx
  shrl  $2, %ecx
  rep   stosl

  mov   %ebx, %cr3
  leal  pa(__boot_pdpt), %eax
  orl   $3, %eax    // P,RW bit
  movl  $((KERNLINK_PA >> 39) & 0x1ff), %edx
  movl  %eax, (%ebx, %edx, 8)
  movl  $((KERNLINK >> 39) & 0x1ff), %edx
  movl  %eax, (%ebx, %edx, 8)

  leal  pa(__boot_pdpt), %ebx
  leal  pa(__boot_pd), %eax
  orl   $3, %eax
  movl  $((KERNLINK_PA >> 30) & 0x1ff), %edx
  movl  %eax, (%ebx, %edx, 8)
  movl  $((KERNLINK >> 30) & 0x1ff), %edx
  movl  %eax, (%ebx, %edx, 8)

  // mapping 0-1 GiB
  leal  pa(__boot_pd), %ebx
  xorl  %ecx, %ecx
setpde:
  movl  $0x200000, %eax   // 2 MiB
  mull  %ecx
  orl   $0b10000011, %eax
  movl  %eax, (%ebx, %ecx, 8)
  incl  %ecx
  cmpl  $512, %ecx
  jne   setpde

  ret

nocpuid:
  movl  $1, %eax
  jmp   booterror
nolongmode:
  movl  $2, %eax
  jmp   booterror
booterror:
  // [NG]
  movl  $0x0e470e4e, (0xb8000)
  hlt

multiboot_magic:
  .quad 0
multiboot_info:
  .quad 0

.p2align 3
entrygdt:
  .quad   0                       // NULL
  .quad   0x00cf9b000000ffff      // KCODE32
  .quad   0x00cf93000000ffff      // KDATA32
  .quad   0x00af9b000000ffff      // KCODE64
  .quad   0x00af93000000ffff      // KDATA64
entrygdt_e:

entrygdt_p:
  .word   entrygdt_e - entrygdt + 1
  .quad   pa(entrygdt)

.code64
.global trampoline64
trampoline64:
  movabs  $entry64, %rax
  jmpq    *%rax

.global entry64
entry64:
  // set data segment
  movw  $(SEG_KDATA << 3), %ax
  movw  %ax, %ds
  movw  %ax, %ss
  movw  %ax, %es
  movw  $0, %ax
  movw  %ax, %fs
  movw  %ax, %gs

  // zero .bss
  cld
  xorl  %eax, %eax
  movl  $pa(__bss_start), %edi
  movl  $pa(__bss_end), %ecx
  subl  %edi, %ecx
  shrl  $2, %ecx
  rep   stosl

  movq  $va(0x7c00), %rax
  movq  %rax, %rsp

  movabs  $multiboot_info, %rax
  movq    (%rax), %rdi
  movabs  $(PAGE_OFFSET), %rax
  addq    %rax, %rdi
  call    x86main

  // never return here!
  cli
  hlt
