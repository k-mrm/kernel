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

struct chan
{
	struct proc *proc;
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
	
	struct tree pn;

	struct list waitq;

	struct list rq;
	struct list free;
	struct list wqn;

	struct chan chan;

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
void sleep(struct chan *chan, int (*cb)(void *), void *arg);
void wakeup(struct chan *chan);


#endif  // _PROC_H
