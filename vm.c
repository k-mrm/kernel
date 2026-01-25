#include <kernel.h>
#include <kalloc.h>
#include <panic.h>
#include <vm.h>
#include <proc.h>
#include <sysmem.h>
#include <string.h>
#include <x86/mm.h>

#define KPREFIX		"vm:"

#include <printk.h>

/*
 *  Kernel address space
 */
static struct vm kvm;

struct vm *
kernelvm(void)
{
	return &kvm;
}

static PTE *
pagewalk (struct vm *vm, ulong va, bool allocpgt)
{
	PageTable pgt = vm->pgdir;
	uint level;
	uint vlevel = vm->level;
	uint vll = vm->lowestlevel;
	ulong pgtpa;
	PTE *pte;

	for (level = vlevel; level > vll; level--) {
		pte = &pgt[PIDX(level, va)];

		if (*pte & ppresent()) {
			pgtpa = PTE_PA (*pte);
			pgt = (PageTable)P2V (pgtpa);
		} else if (allocpgt) {
			pgt = zalloc();
			if (!pgt)
				return NULL;
			pgtpa = V2P(pgt);
			*pte = nexttablepte(pgtpa);
		} else {
			// unmapped
			return NULL;
		}
	}

	return &pgt[PIDX (level, va)];
}

void
mappages(struct vm *vm, ulong va, ulong pa, ulong size, ulong flags, bool remap)
{
        PTE *pte;

        for (ulong p = 0; p < size; p += PAGESIZE, va += PAGESIZE, pa += PAGESIZE) {
                pte = pagewalk (vm, va, true);
                if (!pte)
                        panic ("null pte %p", va);
                if (!remap && (*pte & ppresent()))
                        panic ("this entry has been used: va %p", va);

                *pte = pteleaf(pa, flags);
        }
}

static ulong
addrwalk(struct vm *vm, ulong va)
{
        PTE *pte = pagewalk(vm, va, false);

        if (pte && (*pte & ppresent()))
                return PTE_PA(*pte) | PAGEOFFSET(va);
        else
                return 0;
}

struct vm *
uservm(struct proc *proc)
{
        struct vm *vm;
        void *stack;

        vm = alloc();
        vm->pgdir = zalloc();
        memcpy(vm->pgdir, kvm.pgdir, PAGESIZE);
        vm->level = kvm.level;
        vm->lowestlevel = kvm.lowestlevel;
        vm->user = true;
        vm->proc = proc;

        stack = zalloc();
        mappages(vm, USTACKBOTTOM, V2P(stack), PAGESIZE,
                 pnormal() | pwritable() | puser(), false);
        vm->ustack = stack;
        vm->ustacksize = PAGESIZE;

        vm->ustart = (void *)0x1000;
        vm->csize = 0;
        vm->heap = NULL;
        vm->hsize = 0;

        return vm;
}

void
copyvm(struct proc *proc, struct proc *np)
{
        struct vm *vm = uservm(np);
        PTE *pte;
        ulong pa;
        u64 flags;
        void *p;

        // copy code
        trace ("copyvas %d\n", proc->vm->csize);
        for (u64 i = 0; i < proc->vm->csize; i += PAGESIZE)
        {
                pte = pagewalk (proc->vm, (ulong)proc->vm->ustart + i, true);
                if (!pte)
                        panic ("pte");
                trace("copyvas pte %p\n", pte);

                p = zalloc ();
                if (!p)
                        panic ("p");

                pa = PTE_PA (*pte);
                flags = PTE_FLAGS (*pte);

                trace("copyvas flags %x %x %x\n", *pte, pa, flags);
                memcpy(p, P2V (pa), PAGESIZE);
                mappages(vm, (ulong)proc->vm->ustart + i, V2P (p), PAGESIZE, flags, false);
        }
        // copy heap
        // copy stack
        memcpy(vm->ustack, proc->vm->ustack, vm->ustacksize);

        np->vm = vm;
}

static u64
alignup(u64 va, int *upsz)
{
	if (PAGEALIGNED (va)) {
		*upsz = PAGESIZE;
		return va + PAGESIZE;
	} else {
		*upsz = PAGEALIGN (va) - va;
		return PAGEALIGN (va);
	}
}

void
copyin(struct vm *vm, u64 uva, void *buf, int size)
{
        ulong pa;
        void *p;
        u64 euva = uva + size;
        int sz, offset = 0;

        for (u64 v = uva; v < euva; v = alignup (v, &sz)) {
                pa = addrwalk(vm, v);
                if (pa == 0)
                        return;
                memcpy(P2V (pa), buf + offset, MIN(sz, euva - v));
                offset += MIN (sz, euva - v);
        }
}

void
vmcodealloc(struct vm *vm, u64 sz, u64 flags)
{
        u64 oldsz, newsz;
        void *page, *from, *to, *p;

        oldsz = vm->csize;
        newsz = vm->csize + sz;
        from = vm->ustart + oldsz;
        to = vm->ustart + newsz;

        for (p = (void *)PAGEALIGN (from); (ulong)p < (ulong)to; p += PAGESIZE) {
                page = zalloc ();
                if (!page)
                        panic ("page");
                mappages(vm, p, V2P (page), PAGESIZE, pnormal () | puser () | flags, false);
        }

        vm->csize = newsz;
}


void
freevm(struct vm *vm)
{
        // TODO
        free(vm->pgdir);
        free(vm);
}

void *
devmmap(ulong pa, ulong nbytes)
{
        void *va;
        ulong flags = pdevice() | pwritable() | pnocache();

        // TODO: virtualalloc
        va = alloc();
        if (!va)
                return NULL;
        mappages(&kvm, (ulong)va, pa, PAGESIZE, flags, true);
        return va;
}

static void
initkvm(void)
{
        x86initkvm(&kvm);
        kvm.user = false;
        if (!kvm.pgdir)
                panic ("NULL pgdir");
        if (!PAGEALIGNED(kvm.pgdir))
                panic ("pgdir must be page-aligned");
        memset(kvm.pgdir, 0, PAGESIZE);
}

void
kernelmap(void)
{
        ulong pstart, pend;
        void  *va;
        ulong flags;

        initkvm();

	pstart = PAGEALIGN(memstart());
	pend = PAGEALIGNDOWN(memend());

	for (ulong addr = pstart; addr < pend; addr += PAGESIZE) {
		va = P2V (addr);
		flags = pnormal();
		if (IS_KERN_TEXT(va))
			flags |= preadonly() | pexecutable();
		else if (IS_KERN_RODATA(va))
			flags |= preadonly();
		else
			flags |= pwritable();
		mappages(&kvm, va, addr, PAGESIZE, flags, false);
	}

        switchvm(&kvm);
        trace ("Switched to kernel virtual address space\n");
}
