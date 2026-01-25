#include <kernel.h>
#include <syscall.h>
#include <printk.h>

extern SYSCALL __syscalls_s[];
extern SYSCALL __syscalls_e[];

static void *(*systable[512]) (void *, void *, void *, void *, void *, void *);

void *
syscall (int sysno, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6)
{
        void *(*sc) (void *, void *, void *, void *, void *, void *);

        if (sysno >= 512)
        {
                return (void *)-1;
        }

        sc = systable[sysno];

        if (!sc)
        {
                return (void *)-1;
        }

        return sc (arg1, arg2, arg3, arg4, arg5, arg6);
}

void
syscallinit (void)
{
        SYSCALL *sc;

        for (sc = __syscalls_s; sc < __syscalls_e; sc++)
        {
                systable[sc->sysno] = sc->syscall;
        }
}

static int
sysempty (void)
{
        printk ("hello");
        return -1;
}
SYSCALL_DEFINE (SYS_EMPTY, sysempty);
