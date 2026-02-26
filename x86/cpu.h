#ifndef _X86_CPU_H
#define _X86_CPU_H

#include <kernel.h>

typedef struct X86cpu X86cpu;

struct X86cpu {
  const char      *vendor;
  const char      **id;

  void            (*init) (X86cpu *cpu);
};

void x86cpuinit(void);

#define DEFINE_X86_CPU(_vndr, _id, _init)                                     \
  static USED SECTION (".initdata.x86cpu") ALIGNED (_Alignof (X86cpu))  \
  X86cpu __x86_cpu_ ## _vndr = {                                        \
    .vendor = #_vndr,                                             \
    .id     = _id,                                                \
    .init   = _init,                                              \
  }

#endif  // _X86_CPU_H
