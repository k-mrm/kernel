#ifndef _SYS_H
#define _SYS_H

#include <kernel.h>

typedef enum gatetype gatetype;
typedef struct gatedesc gatedesc;
typedef struct trapframe trapframe;
typedef struct tss tss;
typedef struct e820_entry e820_entry;
typedef struct page page;
typedef struct list list;

// serial.c
void serial_init(void);
int serial_getc(void);
void serial_putc(char c);
// seg.c
void seginit(void);
// trap.c
typedef struct irq irq;
void trapinit(void);
void loadidt(void);
void newirq(int, void (*)(irq*), int (*)(irq*), void*);
// lapic.c
void lapicinit(void);
uint lapicid(void);
void lapiceoi(irq*);
// printk.c
int printk(const char*, ...);
void NORETURN panic(const char*, ...);
// page.c
void pageinit1(ulong);
page *allocpage(void);
void freepage(page*);
// lock.c
typedef struct spinlock spinlock;
typedef struct mutex mutex;
bool trylock(spinlock*);
void lock(spinlock*);
void unlock(spinlock*);
void slockinit(spinlock*);
uint locked(spinlock*);
void mlock(mutex*);
void munlock(mutex*);
void mtxinit(mutex*);
// lib.c
void *memcpy(void *dst, const void *src, ulong n);
void *memmove(void *dst, const void *src, ulong n);
void *memset(void *dst, int c, ulong n);
int memcmp(const void *a, const void *b, uint n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, ulong len);
uint strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strchr(const char *s, int c);
char *strtok(char *s1, const char *s2);
// fs.c
typedef struct superblock superblock;
typedef struct inode inode;
inode *path2inode(char*);
int writei(inode*, void*, uint, uint);
int readi(inode*, void*, uint, uint);
void mountroot(void);
inode *idup(inode*);
void iput(inode*);
inode *iget(superblock*, int);
void isync(inode*);
int bread(superblock*, ulong, void*);
int bwrite(superblock*, ulong, void*);
inode *namei(char*, int);
// ext2.c
void ext2superinit(superblock*);
inode *ext2lookup(inode*, char*);
int ext2iget(inode*);
void ext2sync(inode*);
// wait.c
typedef struct waitpoint waitpoint;
void waitinit(waitpoint*);
// proc.c
typedef enum procstate procstate;
typedef struct proc proc;
typedef struct context context;
typedef struct cpu cpu;
void sleep(waitpoint*, spinlock*);
void wakeup(waitpoint*);
void procinit(void);
void cpu0init(void);
void cpuinit(void);
void schedule(void);
int fork(void);
cpu *mycpu(void);
proc *myproc(void);
int exec(char*);
int exit(int);
int wait(int*);
// kmalloc.c
typedef struct mobj mobj;
void kmallocinit(void);
void *kmalloc(uint);
void kfree(void*);
// vm.c
typedef struct pagetree pagetree;
typedef struct segment segment;
typedef struct pvm pvm;
void kernelmap(void);
void *iomap(ulong base, ulong size);
void segload(segment*, page*);
pvm *procvm(void);
void *vmap(inode*, ulong, ulong, int, uint);
void vmswitch(void);
void vmsegload(segment*, ulong, page*);
void vmseginit(segment*, ulong, ulong);
int faultresume(trapframe*, ulong, int, int);
bool validate(ulong addr, uint len, int wr);
bool validstr(ulong addr, uint limit, int wr);
// block.c
typedef struct block block;
block *getblock(char*);
int __bread();
int __bwrite();
block *newblock(int (*)(block*, int, ulong, int, void*), void*);
// ioapic.c
int ioapicen(int);
void ioapicinit(void);

#endif
