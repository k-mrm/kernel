#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <proc.h>
#include <page.h>
#include <lock.h>
#include <vm.h>
#include <wait.h>
#include <elf.h>
#include <fs.h>

static LIST_HEAD(proctable);
static SPINLOCK(proctable_lk);
static int nextpid = 1;
static LIST_HEAD(cpus);
static LIST_HEAD(runq);
static SPINLOCK(runq_lk);
static proc *proc0;
static cpu cpu0;

void uenter(ulong);

static uint
allocpid(void)
{
  return xadd(&nextpid, 1);
}

static proc *
pid2proc(uint pid)
{
  proc *p;
  lock(&proctable_lk);
  LIST_FOREACH (p, &proctable, ptable) {
    if (p->pid == pid) {
      unlock(&proctable_lk);
      return p;
    }
  }
  unlock(&proctable_lk);
  return NULL;
}

static bool
preemptible(void)
{
  return irq_enabled();
}

// must be disabled preemption
cpu *
mycpu(void)
{
  uint id;
  cpu *c;
  if (preemptible())
    panic("preemptible");
  id = lapicid();
  LIST_FOREACH (c, &cpus, cpulist) {
    if (c->id == id)
      return c;
  }
  panic("cpu!?");
}

proc *
myproc(void)
{
  proc *p;
  u64 flags;
  flags = irqsave();
  p = mycpu()->current;
  irqrestore(flags);
  return p;
}

static void
ready(proc *p)
{
  u64 flags = irqsave();
  lock(&runq_lk);
  p->state = READY;
  lappend(&runq, &p->rq);
  unlock(&runq_lk);
  irqrestore(flags);
}

void
sleep(waitpoint *w, spinlock *lk)
{
  proc *p = myproc();
  u64 flags;
  if (!p)
    panic("sleep");
  lock(&w->lk);
  unlock(lk);
  w->p = p;
  p->state = BLOCKED;
  unlock(&w->lk);
  flags = irqsave();
  swtch(&p->sp, &mycpu()->sched);
  irqrestore(flags);
  lock(lk);
}

void
wakeup(waitpoint *w)
{
  proc *p;
  lock(&w->lk);
  p = w->p;
  if (!p)
    goto end;
  w->p = NULL;
  ready(p);
end:
  unlock(&w->lk);
}

static proc *
allocproc(void)
{
  proc *p;
  p = kmalloc(sizeof *p);
  if (!p)
    return NULL;
  p->kstackpage = allocpage();
  if (!p->kstackpage)
    goto failed;
  p->pid = allocpid();
  p->vm = NULL;
  p->state = NEWBORN;
  lock(&proctable_lk);
  lappend(&proctable, &p->ptable);
  unlock(&proctable_lk);
  return p;
failed:
  kfree(p);
  return NULL;
}

int
exit(int code)
{
  return -1;
}

static void
ktrampoline(void)
{
  proc *p = myproc();
  int r;
  sti();
  r = (*p->kf)(p->ka);
  exit(r);
}

static int
kspawn(const char *name, void *f, void *a)
{
  proc *p;
  void *top;
  context *ctx;
  p = allocproc();
  if (!p)
    return -1;
  strcpy(p->name, name);
  p->kf = f;
  p->ka = a;
  top = pageaddress(p->kstackpage) + PAGESIZE;
  ctx = (context*)(top - sizeof(context));
  ctx->rip = (ulong)ktrampoline;
  p->sp = (ulong)ctx;
  ready(p);
  return 0;
}

static void
freeproc(proc *p)
{
  // TODO
  freepage(p->kstackpage);
  lock(&proctable_lk);
  ldelete(&p->ptable);
  unlock(&proctable_lk);
  kfree(p);
}

int
exec(char *path)
{
  proc *p = myproc();
  page *sp;
  inode *ino;
  pvm *vm = procvm();
  pvm *old = p->vm;
  ehdr eh;
  phdr ph;
  ulong phoff;
  ino = namei(path, O_EXEC);
  if (!ino)
    return -1;
  sp = allocpage();
  if (!sp)
    return -1;
  p->vm = vm;
  vmseginit(stext(p->vm), 0x1000, 0x1000);
  vmseginit(sstack(p->vm), USTACKTOP - 0x1000, 0x1000);
  vmsegload(sstack(p->vm), USTACKTOP - 0x1000, sp);
  mlock(&ino->mtx);
  if (readi(ino, &eh, 0, sizeof eh) != sizeof eh)
    goto err;
  if (!iself(&eh))
    goto err;
  if (eh.e_type != ET_EXEC)
    goto err;
  phoff = eh.e_phoff;
  for (int i = 0; i < eh.e_phnum; i++, phoff += sizeof ph) {
    if (readi(ino, &ph, phoff, sizeof ph) != sizeof ph)
      goto err;
    if (ph.p_type != PT_LOAD)
      continue;
    if (PAGEOFFSET(ph.p_vaddr) != PAGEOFFSET(ph.p_offset))
      goto err;
    if (ph.p_filesz > ph.p_memsz)
      goto err;
    if (!vmap(ino, PAGEALIGNDOWN(ph.p_vaddr), ph.p_filesz,
              VM_READ|VM_EXEC, PAGEALIGNDOWN(ph.p_offset)))
      goto err;
  }
  munlock(&ino->mtx);
  p->tf->rip = eh.e_entry;
  p->tf->rsp = USTACKTOP;
  vmswitch(p);
  return 0;
err:
  panic("torima");
  munlock(&ino->mtx);
  iput(ino);
  return -1;
}

int
fork(void)
{
  proc *p = myproc();
  proc *cp = allocproc();
  void *top;
  context *ctx;
  trapframe *tf;
  if (!cp)
    return -1;
  strcpy(cp->name, p->name);
  top = pageaddress(cp->kstackpage) + PAGESIZE;
  tf = (trapframe*)(top - sizeof(*tf));
  ctx = (context*)(top - sizeof(context));
  // ctx->rip = (ulong)forkret;
  cp->sp = (ulong)ctx;
  ready(cp);
  return 0;
}

void
schedule(void)
{
  proc *p;
  cpu *c = mycpu();
runloop:
  sti();
  for (;;) {
    cli();
    lock(&runq_lk);
    LIST_FOREACH (p, &runq, rq) {
      if (!p->cpu)
        goto found;
    }
    unlock(&runq_lk);
    sti();
    hlt();
  }
found:
  ldelete(&p->rq);
  unlock(&runq_lk);
  p->state = RUNNING;
  c->current = p;
  p->cpu = p->prevcpu = c;
  vmswitch(p);
  swtch(&c->sched, &p->sp);
  c->current = NULL;
  p->cpu = NULL;
  goto runloop;
}

static int
init0(void *_)
{
  page *tp, *sp;
  extern char _binary_uinit_start[];
  extern char _binary_uinit_end[];
  int isize = _binary_uinit_end - _binary_uinit_start;
  proc0 = myproc();
  proc0->vm = procvm();
  if (!proc0->vm)
    panic("init0 vm %d", sizeof(pvm));
  tp = allocpage();
  if (!tp)
    panic("init0 text");
  sp = allocpage();
  if (!sp)
    panic("init0 stack");
  vmseginit(stext(proc0->vm), 0x1000, 0x1000);
  vmseginit(sstack(proc0->vm), USTACKTOP - 0x1000, 0x1000);
  vmsegload(stext(proc0->vm), 0x1000, tp);
  vmsegload(sstack(proc0->vm), USTACKTOP - 0x1000, sp);
  memcpy(pageaddress(tp), _binary_uinit_start, isize);
  vmswitch(proc0);
  mountroot();
  proc0->cwd = path2inode("/");
  uenter(USTACKTOP);
  panic("init0 exit");
}

void
procinit(void)
{
  kspawn("init", init0, NULL);
}

void
cpu0init(void)
{
  cpu0.id = 0;
  cpu0.current = NULL;
  lappend(&cpus, &cpu0.cpulist);
}

void
cpuinit(void)
{
  cpu *c;
  uint id;
  id = lapicid();
  c = kmalloc(sizeof *c);
  if (!c)
    panic("cpu");
  c->id = id;
  c->current = NULL;
  lappend(&cpus, &c->cpulist);
}
