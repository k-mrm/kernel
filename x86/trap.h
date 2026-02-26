#ifndef _X86_TRAP_H
#define _X86_TRAP_H

#include "seg.h"
#include <kernel.h>

#define NR_INTERRUPT 256

#define INT_NMI 2

// Exceptions
#define E_DE 0x0
#define E_DB 0x1
#define E_BP 0x3
#define E_OF 0x4
#define E_BR 0x5
#define E_UD 0x6
#define E_NM 0x7
#define E_DF 0x8
#define E_TS 0xa
#define E_NP 0xb
#define E_SS 0xc
#define E_GP 0xd
#define E_PF 0xe

typedef struct Gatedesc Gatedesc;
typedef enum Gatetype Gatetype;

enum Gatetype {
  GATEDESC_64_INTR = 0xe,
  GATEDESC_64_TRAP = 0xf,
};

/*
 * x86 Gate Descriptor
 */
struct Gatedesc {
  u16 offset_0_15;
  u16 sel;
  u8 ist : 3;
  u8 _rsrv0 : 5;
  u8 gatetype : 4;
  u8 _zero : 1;
  u8 dpl : 2;
  u8 p : 1;
  u16 offset_16_31;
  u32 offset_32_63;
  u32 _rsrv1;
} PACKED;

struct trapframe {
  u64 rax;
  u64 rbx;
  u64 rcx;
  u64 rdx;
  u64 rbp;
  u64 rsi;
  u64 rdi;
  u64 r8;
  u64 r9;
  u64 r10;
  u64 r11;
  u64 r12;
  u64 r13;
  u64 r14;
  u64 r15;
  u64 trapno;
  u64 errcode;
  /* iret */
  u64 rip;
  u64 cs;
  u64 rflags;
  u64 rsp;
  u64 ss;
} PACKED;

void x86trapinit(void);
extern void trapret(void);

#endif // _X86_TRAP_H
