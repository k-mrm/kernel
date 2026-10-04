#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <proc.h>
#include <page.h>
#include <lock.h>
#include <vm.h>

static LIST_HEAD(proctable);
static SPINLOCK(proctable_lk);
static uint nextpid = 1;
static SPINLOCK(pid_lk);
static LIST_HEAD(cpus);
static LIST_HEAD(runq);
static SPINLOCK(runq_lk);
static proc *proc0;

void uenter(ulong);

static uint
allocpid(void)
{
  uint n;
  lock(&pid_lk);
  n = nextpid++;
  unlock(&pid_lk);
  return n;
}

static proc *
pid_to_proc(uint pid)
{
  proc *p;
  LIST_FOREACH (p, &proctable, ptable) {
    if (p->pid == pid)
      return p;
  }
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
sleep(void)
{
  proc *p = myproc();
}

static void
ready(proc *p)
{
  lock(&runq_lk);
  p->state = READY;
  lappend(&runq, &p->rq);
  unlock(&runq_lk);
}

static proc *
allocproc(void)
{
  proc *pr;
  pr = kmalloc(sizeof *pr);
  if (!pr)
    return NULL;
  pr->kstackpage = allocpage();
  if (!pr->kstackpage)
    goto failed;
  pr->pid = allocpid();
  pr->vm = NULL;
  pr->state = NEWBORN;
  lock(&proctable_lk);
  lappend(&proctable, &pr->ptable);
  unlock(&proctable_lk);
  return pr;

failed:
  kfree(pr);
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
freeproc(proc *pr)
{
  kfree(pr);
}

int
fork(void)
{
  ;
}

static bool
runnable(proc *p)
{
  proc *a;
  if (!trylock(&runq_lk))
    return false;
  LIST_FOREACH (a, &runq, rq) {
    if (a == p)
      goto found;
  }
  p = NULL;
found:
  if (!p || p->state == RUNNING) {
    unlock(&runq_lk);
    return false;
  }
  p->state = RUNNING;
  ldelete(&p->rq);
  unlock(&runq_lk);
  return true;
}

void
schedule(void)
{
  proc *p;
  cpu *c = mycpu();
runloop:
  sti();
  for (;;) {
    lock(&runq_lk);
    LIST_FOREACH (p, &runq, rq) {
      goto found;
    }
    unlock(&runq_lk);
    hlt();
  }
found:
  ldelete(&p->rq);
  unlock(&runq_lk);
  cli();
  p->state = RUNNING;
  c->current = p;
  p->prevcpu = c;
  vmswitch(p);
  swtch(&c->sched, &p->sp);
  c->current = NULL;
  goto runloop;
}

void
sched(void)
{
  ;
}

static int
init0(void *_)
{
  page *tp, *sp;
  extern char _binary_uinit_start[];
  extern char _binary_uinit_end[];
  int isize = _binary_uinit_end - _binary_uinit_start;
  printk("init0!\n");
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
  uenter(USTACKTOP);
  panic("init0 exit");
}

void
procinit(void)
{
  printk("spawn init\n");
  kspawn("init", init0, NULL);
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
