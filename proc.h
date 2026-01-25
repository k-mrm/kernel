#ifndef _PROC_H
#define _PROC_H

#include <kernel.h>
#include <cpu.h>
#include <vm.h>
#include <fs.h>
#include <x86/context.h>
#include <x86/trap.h>

enum procstate
{
        NONE,
        READY,
        RUNNING,
        BLOCKING,
        ZOMBIE,
};

struct proc
{
        enum procstate state;
        char pname[32];

        struct vm *vm;    // address space
        uint procid;
        struct cpu *cpu;
        struct trapframe *tf;
        struct context context;
        void *kstack;
        void *ksp;

        struct inode *cwd;
	
	void *chan;

	struct tree pn;

	struct list waitq;

	struct list rq;
	struct list free;
	struct list wqn;

        int exitstatus;

        bool user;
        // for kernel process
        int (*func)(void *arg);
        void *arg;
};

void init_process(void);
int idleprocess(void *a);
int spawn(char *pname, struct proc *parent, int (*proc)(void *arg), void *arg);
int killpid(uint pid);
int kill(char *pname);
void schedule(void);
int exit(int status);


#endif  // _PROC_H
