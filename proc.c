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

typedef struct procinode procinode;
typedef int (*procop)(procinode*, void*, uint, uint);

struct procinode {
  procinode *ppi;
  u16 mode;
  char name[16];
  uint inum;
  list children;
  list cent;
  int pid;
  int (*readop)(procinode*, void*, uint, uint);
  int (*writeop)(procinode*, void*, uint, uint);
};

static LIST_HEAD(proctable);
static SPINLOCK(proctable_lk);
static int nextpid = 1;
static LIST_HEAD(cpus);
static LIST_HEAD(runq);
static SPINLOCK(runq_lk);
static proc *proc0;
static cpu cpu0;
static superblock *procsuper;
static procinode *procroot;
static int nextprinum = 2;

void uenter(ulong);
void forkret(void);

static int
procinumalloc(void)
{
  return xadd(&nextprinum, 1);
}

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
  if (p->state == READY)
    panic("double ready pid=%d name=%s\n", p->pid, p->name);
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
  lappend(&w->waiters, &p->wq);
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
  proc *p, *tp;
  lock(&w->lk);
  LIST_FOREACH_SAFE (p, tp, &w->waiters, wq) {
    ldelete(&p->wq);
    ready(p);
  }
  unlock(&w->lk);
}

static proc *
allocproc(void)
{
  proc *p;
  inode *rooti;
  inode *ino;
  char name[16] = {0};
  p = kmalloc(sizeof *p);
  if (!p)
    return NULL;
  p->kstackpage = allocpage();
  if (!p->kstackpage)
    goto failed;
  p->pid = allocpid();
  p->vm = NULL;
  p->state = NEWBORN;
  waitinit(&p->wait);
  linit(&p->exitq);
  slockinit(&p->lk);
  lock(&proctable_lk);
  lappend(&proctable, &p->ptable);
  unlock(&proctable_lk);
  if (procsuper) {
    sprintf(name, "%d", p->pid);
    rooti = procsuper->rooti;
    mlock(&rooti->mtx);
    ino = createi(rooti, name, S_IRUSR|S_IFDIR);
    munlock(&rooti->mtx);
    if (ino) {
      mlock(&ino->mtx);
      createi(ino, "name", S_IRUSR);
      createi(ino, "intr", S_IWUSR);
      createi(ino, "stat", S_IRUSR);
      munlock(&ino->mtx);
    }
  }
  return p;
failed:
  kfree(p);
  return NULL;
}

int
wait(int *status)
{
  proc *p = myproc();
  uint cpid;
  defunct *d;
  lock(&p->lk);
  if (p->nchild == 0 && lempty(&p->exitq)) {
    unlock(&p->lk);
    return -1;
  }
  while (lempty(&p->exitq))
    sleep(&p->wait, &p->lk);
  d = LIST_TOP(&p->exitq, defunct, ent);
  ldelete(&d->ent);
  unlock(&p->lk);
  if (status)
    *status = d->exitcode;
  cpid = d->pid;
  kfree(d);
  return cpid;
}

int
exit(int code)
{
  proc *p = myproc();
  proc *pp;
  defunct *d;
  p->exitcode = code;
  for (int i = 0; i < 16; i++) {
    if (p->openf[i]) {
      close(p->openf[i]);
      p->openf[i] = NULL;
    }
  }
  if (p->kf)
    goto kexit;
  pp = p->parent;
  if (!pp)
    panic("exit");
  d = kmalloc(sizeof *d);
  d->exitcode = code;
  d->pid = p->pid;
  lock(&pp->lk);
  pp->nchild--;
  lappend(&pp->exitq, &d->ent);
  wakeup(&pp->wait);
  unlock(&pp->lk);
kexit:
  p->state = DEAD;
  cli();
  swtch(&p->sp, &mycpu()->sched);
  panic("exit");
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

static const char *
statestr(procstate s)
{
  switch (s) {
    case NULLSTATE: return "null";
    case NEWBORN:   return "newborn";
    case RUNNING:   return "running";
    case READY:     return "ready";
    case BLOCKED:   return "blocked";
    case DEAD:      return "dead";
  }
  return "?";
}

void
pdump(proc *p)
{
  int cpuid;
  int ppid;
  cpuid = p->cpu ? (int)p->cpu->id : -1;
  ppid = p->parent ? (int)p->parent->pid : 0;
  printk("%d\t%s\t%d\t%d\t%d\t%s\n",
         p->pid, statestr(p->state), cpuid, ppid, p->nchild, p->name);
}

void
ptabledump()
{
  proc *p;
  printk("pid    state     cpu  ppid  nchild  name\n");
  lock(&proctable_lk);
  LIST_FOREACH (p, &proctable, ptable)
    pdump(p);
  unlock(&proctable_lk);
}

int
exec(char *path, char **argv)
{
  proc *p = myproc();
  page *sppage;
  void *sp = (void*)USTACKTOP;
  inode *ino;
  pvm *vm = procvm();
  pvm *old = p->vm;
  ehdr eh;
  phdr ph;
  ulong phoff;
  uint perm;
  ulong ustack[16+1];
  uint argc;
  ino = namei(path, O_EXEC);
  if (!ino)
    return -1;
  sppage = allocpage();
  if (!sppage)
    return -1;
  p->vm = vm;
  vmstackinit(USTACKTOP - 0x1000, 0x1000);
  vmload(USTACKTOP - 0x1000, sppage);
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
    perm = ((ph.p_flags & PF_X) ? VM_EXEC : 0) |
           ((ph.p_flags & PF_W) ? VM_WRITE : 0) |
           ((ph.p_flags & PF_R) ? VM_READ : 0);
    if (!vmap(ino, ph.p_vaddr, ph.p_filesz, ph.p_memsz, perm, ph.p_offset))
      goto err;
  }
  munlock(&ino->mtx);
  iput(ino);
  vmswitch();
  for (argc = 0; argv[argc]; argc++) {
    sp = (void*)((ulong)(sp - (strlen(argv[argc]) + 1)) & ~7);
    memcpy(sp, argv[argc], strlen(argv[argc]) + 1);
    ustack[argc] = sp;
    kfree(argv[argc]);
  }
  ustack[argc] = 0;
  sp -= (argc+1) * sizeof(ulong);
  memcpy(sp, ustack, (argc+1) * sizeof(ulong));
  strcpy(p->name, path);
  p->tf->rip = eh.e_entry;
  p->tf->rsp = (ulong)sp;
  p->tf->rdi = argc;
  p->tf->rsi = (ulong)sp;
  // freevm(old);
  return 0;
err:
  panic("torima");
  munlock(&ino->mtx);
  iput(ino);
  return -1;
}

void
schedtail(void)
{
  ; // do nothing
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
  ctx = (context*)((void*)tf - sizeof(context));
  memcpy(tf, p->tf, sizeof *tf);
  tf->rax = 0;
  ctx->rip = (ulong)forkret;
  cp->sp = (ulong)ctx;
  cp->cwd = idup(p->cwd);
  cp->vm = vmdup();
  if (!cp->vm)
    return -1;
  for (int i = 0; i < 16; i++)
    if (p->openf[i])
      cp->openf[i] = fdup(p->openf[i]);
  p->nchild++;
  cp->parent = p;
  ready(cp);
  return cp->pid;
}

void
yield(void)
{
  proc *p;
  if (preemptible())
    panic("yield: preemptible");
  p = myproc();
  if (!p)
    panic("yield");
  mycpu()->yield = 0;
  swtch(&p->sp, &mycpu()->sched);
}

static void
reap(void)
{
  proc *p = myproc();
  p->cpu = NULL;
  if (p->state == RUNNING) {
    ready(p);
  } else if (p->state == DEAD) {
    freeproc(p);
  }
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
      if (p->state != READY) {
        ptabledump();
        pdump(p);
        panic("rq");
      }
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
  vmswitch();
  swtch(&c->sched, &p->sp);
  reap();
  c->current = NULL;
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
  vmtextinit(0x1000, 0x1000);
  vmstackinit(USTACKTOP - 0x1000, 0x1000);
  if (vmload(0x1000, tp) < 0)
    panic("init0 tp");
  if (vmload(USTACKTOP - 0x1000, sp) < 0)
    panic("init0 sp");
  memcpy(pageaddress(tp), _binary_uinit_start, isize);
  vmswitch();
  mountroot();
  vmount();
  proc0->cwd = path2inode("/");
  uenter(USTACKTOP);
  panic("init0 exit");
}

void
procinit(void)
{
  kspawn("init0", init0, NULL);
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

static int
getpid(procinode *pi, void *buf, uint off, uint n)
{
  proc *p = myproc();
  uint l, c = 0;
  char tmp[32];
  l = sprintf(tmp, "%d", p->pid);
  n = MIN(l, n);
  for (uint i = off; i < n; i++, c++) {
    *((char*)buf + c) = tmp[i];
  }
  return c;
}

static int
getppid(procinode *pi, void *buf, uint off, uint n)
{
  proc *p = myproc();
  proc *pp = p->parent;
  uint l, c = 0;
  char tmp[32];
  l = sprintf(tmp, "%d", pp->pid);
  n = MIN(l, n);
  for (uint i = off; i < n; i++, c++) {
    *((char*)buf + c) = tmp[i];
  }
  return c;
}

static const char*
procstatestr(procstate s)
{
  switch (s) {
  case NEWBORN:  return "newborn";
  case RUNNING:  return "running";
  case READY:    return "ready";
  case BLOCKED:  return "blocked";
  case DEAD:     return "dead";
  default:       return "null";
  }
}

static int
getprocname(procinode *pi, void *buf, uint off, uint n)
{
  proc *p = pid2proc(pi->ppi->pid);
  uint l, c = 0;
  char tmp[32];
  if (!p)
    return -1;
  l = sprintf(tmp, "%s", p->name);
  n = MIN(l, n);
  for (uint i = off; i < n; i++, c++) {
    *((char*)buf + c) = tmp[i];
  }
  return c;
}

static int
getprocstat(procinode *pi, void *buf, uint off, uint n)
{
  proc *p = pid2proc(pi->ppi->pid);
  uint l, c = 0;
  char tmp[64];
  if (!p)
    return -1;
  l = sprintf(tmp, "%d %d %s %s", p->pid, p->parent ? p->parent->pid : 0,
              procstatestr(p->state), p->name);
  n = MIN(l, n);
  for (uint i = off; i < n; i++, c++) {
    *((char*)buf + c) = tmp[i];
  }
  return c;
}

static uint
atou(const char *name)
{
  int len = strlen(name);
  uint n = 0, c;
  if (len > 1 && *name == '0')
    return ~0u;
  do {
    c = *name++ - '0';
    if (c > 9)
      return ~0u;
    n *= 10;
    n += c;
  } while (--len > 0);
  return n;
}

static inode*
procilookup(inode *pi, char *name)
{
  superblock *sb = pi->super;
  procinode *ppi = pi->data;
  procinode *p;
  inode *ino;
  LIST_FOREACH (p, &ppi->children, cent) {
    if (strcmp(p->name, name) == 0) {
      ino = iget(sb, p->inum);
      if (!ino)
        panic("procilookup");
      return ino;
    }
  }
  return NULL;
}

static procinode*
allocprocinode(procinode *ppi, char *name, u16 mode)
{
  procinode *p = kmalloc(sizeof *p);
  p->mode = mode;
  strcpy(p->name, name);
  p->inum = procinumalloc();
  linit(&p->children);
  lappend(&ppi->children, &p->cent);
  p->ppi = ppi;
  return p;
}

static int
getprocrootop(const char *name, procop *rop, procop *wop)
{
  if (strcmp(name, "pid") == 0) {
    *rop = getpid; *wop = NULL;
    return 0;
  } else if (strcmp(name, "ppid") == 0) {
    *rop = getppid; *wop = NULL;
    return 0;
  }
  return -1;
}

static int
getprocop(const char *name, procop *rop, procop *wop)
{
  if (strcmp(name, "name") == 0) {
    *rop = getprocname; *wop = NULL;
    return 0;
  } else if (strcmp(name, "intr") == 0) {
    *rop = NULL; *wop = NULL;
    return 0;
  } else if (strcmp(name, "stat") == 0) {
    *rop = getprocstat; *wop = NULL;
    return 0;
  }
  return -1;
}

static int
procreadi(inode *ino, void *buf, uint off, uint n)
{
  procinode *pi = ino->data;
  if (!pi->readop)
    return -1;
  return pi->readop(pi, buf, off, n);
}

static int
procwritei(inode *ino, void *buf, uint off, uint n)
{
  procinode *pi = ino->data;
  if (!pi->writeop)
    return -1;
  return pi->writeop(pi, buf, off, n);
}

static int
procreaddir(inode *pi, dentry *dent)
{
  uint n = 0;
  superblock *sb = pi->super;
  procinode *ppi = pi->data;
  procinode *p;
  LIST_FOREACH (p, &ppi->children, cent) {
    if (dent->n == n) {
      dent->ino = iget(sb, p->inum);
      if (!dent->ino)
        panic("procreaddir");
      memcpy(dent->name, p->name, sizeof(p->name));
      dent->namelen = strlen(p->name);
      return 1;
    }
    n++;
  }
  return 0;
}

static inode*
proccreatei(inode *pari, char *name, u16 mode)
{
  superblock *sb = pari->super;
  procinode *ppi = pari->data;
  procinode *pi;
  inode *ino;
  pi = allocprocinode(ppi, name, mode);
  if (!pi)
    return NULL;
  getprocop(name, &pi->readop, &pi->writeop);
  ino = iget(sb, pi->inum);
  if (!ino)
    return NULL;
  ino->mode = pi->mode;
  ino->nlink = 1;
  ino->size = 0;
  ino->blocks = 0;
  ino->readi = procreadi;
  ino->writei = procwritei;
  if (S_ISDIR(pi->mode)) {
    ino->readdir = procreaddir;
    ino->lookup = procilookup;
    ino->create = proccreatei;
  }
  ino->data = pi;
  return ino;
}

static inode*
procrootcreatei(inode *rooti, char *name, u16 mode)
{
  superblock *sb = rooti->super;
  inode *ino;
  procinode *pi, *rootpi = rooti->data;
  proc *proc;
  uint pid;
  pi = allocprocinode(rootpi, name, mode);
  if (!pi)
    return NULL;
  pid = atou(name);
  if (pid == ~0u) {
    pi->pid = -1;
    getprocrootop(name, &pi->readop, &pi->writeop);
  } else {
    proc = pid2proc(pid);
    if (!proc)
      goto out;
    pi->pid = pid;
    pi->readop = pi->writeop = NULL;
  }
  ino = iget(sb, pi->inum);
  if (!ino)
    goto out;
  ino->mode = pi->mode;
  ino->nlink = 1;
  ino->size = 0;
  ino->blocks = 0;
  ino->readi = procreadi;
  ino->writei = procwritei;
  if (S_ISDIR(pi->mode)) {
    ino->readdir = procreaddir;
    ino->lookup = procilookup;
    ino->create = proccreatei;
  }
  ino->data = pi;
  return ino;
out:
  // free pi;
  return NULL;
}

superblock*
procsuperblock(void)
{
  inode *rooti;
  procsuper = newsb("proc", NULL);
  if (!procsuper)
    panic("f");
  procsuper->bsize = 0;
  procsuper->fssuper = NULL;
  // /proc
  procroot = kmalloc(sizeof *procroot);
  if (!procroot)
    panic("procroot");
  procroot->mode = S_IFDIR | S_IRUSR | S_IWUSR;
  procroot->inum = 1;
  linit(&procroot->children);
  procroot->ppi = NULL;
  rooti = iget(procsuper, 1);
  if (!rooti)
    panic("procsuper");
  procsuper->rooti = rooti;
  rooti->mode = procroot->mode;
  rooti->nlink = 1;
  rooti->size = 0;
  rooti->blocks = 0;
  rooti->readi = procreadi;
  rooti->writei = procwritei;
  rooti->readdir = procreaddir;
  rooti->lookup = procilookup;
  rooti->create = procrootcreatei;
  rooti->data = procroot;
  createi(rooti, "pid", S_IRUSR);
  createi(rooti, "ppid", S_IRUSR);
  return procsuper;
}
