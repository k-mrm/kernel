#include <kernel.h>
#include <vm.h>
#include <cpu.h>
#include <kalloc.h>
#include <string.h>
#include <proc.h>
#include <elf.h>
#include <syscall.h>
#include <timer.h>
#include <panic.h>
#include <x86/arch.h>
#include <x86/mm.h>
#include <x86/trap.h>
#include <x86/seg.h>

#define KPREFIX       "proc:"

#include <printk.h>

static LIST_HEAD(rq);
static struct proc *kidle;
static struct proc *initproc;
static uint procidtable = 0;

// Locked r
static struct proc *
rqpop(void)
{
        struct proc *p;

        if (list_empty(&rq))
                return NULL;
        p = LIST_ENTRY(&rq, struct proc, rq);
        list_delete(&p->rq);

        return p;
}

static uint
procid (void)
{
        return procidtable++;
}

static void
initnewproctf(struct proc *p, struct trapframe *tf)
{
        if (p->user) {
                u64 rflags;
                asm volatile (
                        "pushfq\n"
                        "pop  %0\n" : "=r" (rflags)
                );

                tf->rip     = 0x1000;
                tf->cs      = (SEG_UCODE << 3) | DPL_USER;
                tf->rflags  = rflags | EFLAGS_IF;
                tf->ss      = (SEG_UDATA << 3) | DPL_USER;
                tf->rsp     = USTACKTOP;
        } else {
                u64 cs, rflags, ss;
                asm volatile ("mov  %%cs, %0" : "=r" (cs));
                asm volatile ("mov  %%ss, %0" : "=r" (ss));
                asm volatile (
                        "pushfq\n"
                        "pop  %0\n" : "=r" (rflags)
                );

                tf->rip     = (u64)p->func;
                tf->rdi     = (u64)p->arg;
                tf->cs      = cs;
                tf->rflags  = rflags | EFLAGS_IF;
                tf->ss      = ss;
                tf->rsp     = (u64)tf;
        }
}

static void
dead(struct proc *p)
{
        if (!p)
                return;
        p->state = ZOMBIE;
}

static void
ready(struct proc *p)
{
        if (!p)
                return;
        // Lock rq
        p->state = READY;
	list_add(&rq, &p->rq);
        // Unlock rq
}

// Locked rq
static void
running(struct proc *p)
{
        if (!p)
                return;
        // assert (rq is locked)
        p->state = RUNNING;
}

static void
block(struct proc *p)
{
        if (!p)
                return;
        p->state = BLOCKING;
}

static struct proc *
newproc(char *name, struct proc *parent, bool user, int (*pfunc) (void *arg), void *parg)
{
        struct proc *p = zalloc ();
        void *sp;

        if (!p)
                return NULL;

        p->procid = procid ();

        p->user = user;
        if (!p->user) {
                p->func = pfunc;
                p->arg  = parg;
        }

        p->vm = NULL;

	initlist(&p->waitq);
	inittree(&p->pn);
	if (parent)
		new_child(&parent->pn, &p->pn);

        strcpy(p->pname, name);

        p->kstack = zalloc();
        if (!p->kstack)
                goto err;
        sp = p->kstack + PAGESIZE;
        // init trapframe
        sp -= sizeof *p->tf;
        initnewproctf(p, sp);
        p->tf = sp;
        log("p->tf->sp:%p, ss:%p, rflags:%p, cs:%p, rip:%p\n",
            p->tf->rsp, p->tf->ss, p->tf->rflags, p->tf->cs, p->tf->rip);
        // init stackframe for context switch
        sp -= sizeof (struct stackframe);
        ((struct stackframe *)sp)->rip = (u64)forkret;

        p->context.rsp  = (u64)sp;
        return p;

err:
        free (p);
        return NULL;
}

static void
freeproc (struct proc *p)
{
        free(p->kstack);
        memset (p, sizeof *p, 0);
        free (p);
}

static void
inituserproc(void)
{
        struct proc *p;
        extern char _binary_initcode_start[];
        extern char _binary_initcode_end[];
        int isize = _binary_initcode_end - _binary_initcode_start;
        void *initcode;

        p = newproc("init0", NULL, true, NULL, NULL);
        if (!p)
                return;

        p->vm = uservm(p);
        p->cwd = path2ino ("/");

        initcode = zalloc();
        memcpy (initcode, _binary_initcode_start, isize);
        mappages (p->vm, 0x1000, V2P (initcode), PAGESIZE,
                  pnormal () | preadonly () | pexecutable () | puser (), false);

        ready(p);

	initproc = p;
}

void
initprocess (void)
{
        spawn ("kidle", NULL, idleprocess, NULL);

        inituserproc();
}

static int
exec (const char *path, const char **argv)
{
        struct inode *elf;
        struct fs *fs;
        struct ehdr ehdr;
        struct phdr phdr;
        int status, size = 0;
        u64 phoff, flags;
        struct cpu *cpu = mycpu();
        struct proc *proc = cpu->current;
        void *p;
        int uargc = 0;
        struct vm *vm = uservm(proc);
        struct vm *oldvm = proc->vm;
        void *top = vm->ustack + PAGESIZE;
        void *sp = top;
        ulong args[9];

	memset(args, 0, sizeof(ulong) * 9);

        elf = path2ino(path);
        if (!elf)
                goto err;

        fs = elf->fs;
        status = fs->op->readi(elf, (uchar *)&ehdr, 0, sizeof ehdr);
        if (status != sizeof (ehdr))
                goto err;
        if (!iself (&ehdr))
                goto err;
        if (ehdr.e_type != ET_EXEC)
                goto err;

        phoff = ehdr.e_phoff;
        for (int i = 0; i < ehdr.e_phnum; i++, phoff += sizeof phdr) {
                p = zalloc ();
                flags = 0;

                status = fs->op->readi (elf, (uchar *)&phdr, phoff, sizeof phdr);
                if (status != sizeof phdr)
                        goto err;
                if (phdr.p_type != PT_LOAD)
                        continue;

                if (!PAGEALIGNED (phdr.p_vaddr))
                        panic ("o");

                flags |= phdr.p_flags & PF_X ? pexecutable () | preadonly () : 0;
                flags |= phdr.p_flags & PF_W ? pwritable () : 0;

		vmcodealloc(vm, phdr.p_memsz, flags);

		size = fs->op->readi(elf, p, phdr.p_offset, phdr.p_filesz);
		trace("exec copyin %p %d\n", phdr.p_vaddr, size);
		copyin(vm, phdr.p_vaddr, p, size);
	}

        // setup arguments
        for (; argv && argv[uargc]; uargc++) {
                sp -= strlen (argv[uargc]) + 1;
                sp = (void *)((u64)sp & ~0xf);
                if (sp < vm->ustack)
                        goto err;

                memcpy (sp, argv[uargc], strlen (argv[uargc]));
                args[uargc] = USTACKTOP - (top - sp);
        }

        sp -= sizeof (args[0]) * (uargc + 1);
        sp = (void *)((u64)sp & ~0xf);
        if (sp < vm->ustack)
                goto err;
        memcpy (sp, args, sizeof (args[0]) * uargc);

        proc->tf->rip = ehdr.e_entry;
        proc->tf->rdi = uargc;
        proc->tf->rsi = USTACKTOP - (top - sp);
        proc->tf->rsp = USTACKTOP - (top - sp);
        proc->vm = vm;

        trace ("exec %s rip %#x rdi %d rsp %#x\n", path, proc->tf->rip, proc->tf->rdi, proc->tf->rsp);

        freevm(oldvm);
        switchvm(proc->vm);

        return 0;
err:
        panic ("err");
        freevm(vm);
        return -1;
}

static int
sysexec (const char * USER path, const char ** USER argv)
{
        // TODO: copyuser
        return exec (path, argv);
}

SYSCALL_DEFINE (SYS_EXEC, sysexec);

static int
fork(void)
{
	struct cpu *cpu = mycpu();
	struct proc *proc = cpu->current;
	struct proc *np = newproc(proc->pname, proc, true, NULL, NULL);

	if (!np)
		return -1;

	trace("fork %d %s %p %p\n", np->procid, proc->pname, np->tf, proc->tf);

	memcpy(np->tf, proc->tf, sizeof *np->tf);
	np->tf->rax = 0;
	np->cwd = idup(proc->cwd);
	copyvm(proc, np);

	ready(np);

	return np->procid;
}

SYSCALL_DEFINE (SYS_FORK, fork);

int NORETURN
idleprocess (void *a)
{
        for (;;)
                HLT;
}

int
spawn(char *pname, struct proc *parent, int (*pfunc) (void *arg), void *parg)
{
        struct proc *p;

        p = newproc (pname, parent, false, pfunc, parg);
        if (!p)
                return -1;

        if (strcmp (pname, "kidle") == 0) {
                kidle = p;
                return 0;
        }

        ready (p);
        return 0;
}

void
sleep(void *chan)
{
	struct proc *p = mycpu()->current;

	p->chan = chan;
	block(p);

	schedule();

	p->chan = NULL;
}

static void *
__wakeup(struct tree *t, void *chan)
{
	struct proc *p = container_of(t, struct proc, pn);
	if (p->state == BLOCKING && p->chan == chan)
		ready(p);
	return NULL;
}

void
wakeup(void *chan)
{
	tree_dfs(&initproc->pn, __wakeup, chan);
}


int
exit(int status)
{
        struct cpu *cpu = mycpu ();
        struct proc *proc = cpu->current;
	struct proc *pp = PARENT(&proc->pn, struct proc, pn);

        proc->exitstatus = status;
        dead(proc);

        if (pp) {
		list_add(&pp->waitq, &proc->wqn);
		wakeup(pp);
	}

        schedule();
        // never return here
	return -1;
}

SYSCALL_DEFINE(SYS_EXIT, exit);

static bool
wqempty(struct proc *proc)
{
        return list_empty(&proc->waitq);
}

int
wait(int *status)
{
        struct cpu *cpu = mycpu ();
        struct proc *proc = cpu->current;
        struct proc *p;
        int pid;

        if (!nchild(&proc->pn)) {
                // no child
                return -1;
        }

        while (wqempty (proc))
                sleep (proc);

        p = LIST_ENTRY(&proc->waitq, struct proc, wqn);
        list_delete(&p->wqn);
        if (p->state != ZOMBIE)
                panic("non zombie");
        pid = p->procid;
        if (status) {
                // xxx: use copyout
                *status = p->exitstatus;
        }
        freeproc(p);
        return pid;
}

SYSCALL_DEFINE(SYS_WAIT, wait);

static struct proc *
nextproc(struct proc *prev)
{
        struct proc *p;

        if (prev && prev->state == RUNNING)
                ready(prev);

        p = rqpop();
        if (!p)
                return kidle;
        return p;
}

void
schedtail(void)
{
        return;
}

void
schedule (void)
{
        struct cpu *cpu = mycpu ();
        struct proc *prev = cpu->current;
        struct proc *next = nextproc(prev); 
        struct context *c;

        if (!next)
                panic ("next process!?");

        cpu->current = next;
        next->cpu = cpu;
        running(next);

        log ("cswitch: prev %p(%s) -> %p(%s)\n",
             prev, prev ? prev->pname : "NULL", next, next->pname);

        if (UNLIKELY(!prev)) {
                // from kernel
                c = &cpu->context;
        } else { 
                // from process
                c = &prev->context;
        }

        if (next->vm)
                switchvm(next->vm);

        prev = cswitch(c, &next->context, prev);

        log ("cswitch returned: last:%s c:%s %d\n",
             prev ? prev->pname : "kernel", cpu->current->pname, interruptible());
}
