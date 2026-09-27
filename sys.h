#include <kernel.h>

typedef enum gatetype gatetype;
typedef struct gatedesc gatedesc;
typedef struct trapframe trapframe;
typedef struct tss tss;
typedef struct e820_entry e820_entry;
typedef struct page page;
typedef struct irq irq;
typedef struct list list;
typedef struct procvm procvm;

// serial.c
void serial_init(void);
int serial_getc(void);
void serial_putc(char c);
// seg.c
void seginit(void);
// trap.c
void trapinit(void);
void newirq(int irqno, void (*eoi)(irq*), int (*handle)(irq*));
// printk.c
int printk(const char *fmt, ...);
void NORETURN panic(const char *fmt, ...);
// page.c
void pageinit1(ulong);
page *allocpage(void);
void freepage(page*);
// vm.c
void kernelmap(void);
void *iomap(ulong base, ulong size);
// lib.c
void *memcpy(void *dst, const void *src, ulong n);
void *memmove(void *dst, const void *src, ulong n);
void *memset(void *dst, int c, ulong n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, ulong len);
uint strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strchr(const char *s, int c);
char *strtok(char *s1, const char *s2);
// proc.c
typedef enum procstate procstate;
typedef struct proc proc;
typedef struct context context;
typedef struct cpu cpu;
void procinit(void);
void cpuinit(void);
cpu *mycpu(void);
proc *myproc(void);
// kmalloc.c
typedef struct mobj mobj;
void kmallocinit(void);
void *kmalloc(uint);
void kfree(void*);
// lock.c
typedef struct spinlock spinlock;
bool trylock(spinlock*);
void lock(spinlock*);
void unlock(spinlock*);
void slockinit(spinlock*);
