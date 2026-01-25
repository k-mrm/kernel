#ifndef _X86_CONTEXT_H
#define _X86_CONTEXT_H

#include <kernel.h>

struct stackframe {
	u64   r15;
	u64   r14;
	u64   r13;
	u64   r12;
	u64   rbx;
	u64   rbp;
	u64   rip;
} PACKED;

struct context {
	u64   rsp;    // must be fast!
};

void forkret (void);
struct proc *cswitch(struct context *prev, struct context *next, struct proc *pprev);

#endif  // _X86_CONTEXT_H
