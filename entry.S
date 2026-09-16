#include <asm.h>

#define pa(x)       (x - PAGE_OFFSET)
#define va(x)       (x + PAGE_OFFSET)

.section ".text.boot"

.code16
.global entry16
entry16:
  cli

  call  gete820

  addr32 lgdtl pa(entrygdt_p)
  movl  %cr0, %eax
  orl   $CR0_PE, %eax
  movl  %eax, %cr0
  ljmpl $(SEG_KCODE32 << 3), $pa_entry32

gete820:
  xorl  %ebx, %ebx
  movw  $(pa(e820) - 0x20000), %di
  xorw  %si, %si
1:
  movl  $0xe820, %eax
  movl  $0x534d4150, %edx
  movl  $20, %ecx
  int   $0x15
  jc    2f
  cmpl  $0x534d4150, %eax
  jne   2f
  addw  $20, %di
  incw  %si
  testl %ebx, %ebx
  jnz   1b
2:
  addr32 mov %si, pa(e820count)
  ret

.code32
.global entry32
entry32:
  movw  $(SEG_KDATA32 << 3), %ax
  movw  %ax, %ds 
  movw  %ax, %es 
  movw  %ax, %ss 
  xorw  %ax, %ax
  movw  %ax, %fs
  movw  %ax, %gs

  // init sp
  movl  $0x7c00, %eax
  movl  %eax, %esp
  
  call	cpuverify
  call	initpic
  
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
  
  // enable paging
  mov   %cr0, %eax
  orl   $(CR0_PE | CR0_PG), %eax
  mov   %eax, %cr0
  
  ljmpl $(SEG_KCODE << 3), $pa_trampoline64
  
  hlt

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

initpic:
  mov	$0xff, %al
  outb	%al, $0x21
  mov	$0xff, %al
  outb	%al, $0xa1
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
	movl	$1, %eax
	jmp	booterror
nolongmode:
	movl	$2, %eax
	jmp	booterror
booterror:
	hlt

.global e820count, e820
e820count:
  .quad 0
e820:
  .space 20 * 32

.p2align 3
entrygdt:
	.quad	0                       // NULL
	.quad	0x00cf9b000000ffff      // KCODE32
	.quad	0x00cf93000000ffff      // KDATA32
	.quad	0x00af9b000000ffff      // KCODE64
	.quad	0x00af93000000ffff      // KDATA64
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
  call  bspmain
  // never return here!
  cli
  hlt
1:
  jmp 1b
