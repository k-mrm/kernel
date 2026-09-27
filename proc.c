#include <kernel.h>
#include <asm.h>
#include <sys.h>
#include <proc.h>
#include <page.h>

static LIST_HEAD(proctable);
// static SPINLOCK(proctable_lk);
static uint nextpid = 1;
// static SPINLOCK(pid_lk);
static LIST_HEAD(cpus);
static LIST_HEAD(runq);
// static SPINLOCK(runq_lk);

static uint
allocpid(void)
{
  uint n;
  // lock(&pid_lk);
  n = nextpid++;
  // unlock(&pid_lk);
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
static cpu *
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

static proc *
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
  // lock(&runq_lk);
  p->state = READY;
  lappend(&runq, &p->rq);
  // unlock(&runq_lk);
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
  // lock(&proctable_lk);
  lappend(&proctable, &pr->ptable);
  // unlock(&proctable_lk);
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
  /*
  if (!trylock(&runq_lk))
    return false;
    */
  proc *a;
  LIST_FOREACH (a, &runq, rq) {
    if (a == p)
      goto found;
  }
  p = NULL;
found:
  if (!p || p->state == RUNNING) {
    // unlock(&runq_lk);
    return false;
  }
  p->state = RUNNING;
  ldelete(&p->rq);
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
    LIST_FOREACH (p, &runq, rq) {
      goto found;
    }
  }
found:
  cli();
  if (!runnable(p))
    goto runloop;
  c->current = p;
  p->prevcpu = c;
  // if (p->vm)
  //  vmproc(p->vm);
  swtch(&c->sched, &p->sp);
  // vmkernel();
  c->current = NULL;
  // unlock(&runq_lk);
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
  printk("init0!\n");
  for (;;)
    ;
  return 0;
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
