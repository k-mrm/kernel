// %rdi: context *prev 
// %rsi: context *next
// %rdx: proc *pprev
.text
.global swtch
swtch: 
  pushq   %rbp
  pushq   %rbx
  pushq   %r12
  pushq   %r13
  pushq   %r14
  pushq   %r15
  
  movq    %rsp, (%rdi)
  movq    (%rsi), %rsp

  popq    %r15
  popq    %r14
  popq    %r13
  popq    %r12
  popq    %rbx
  popq    %rbp

  jmp     __cswitch

.global forkret
forkret:
  // call    schedtail
  jmp     trapret
