#include <kernel.h>

typedef enum gatetype gatetype;
typedef struct gatedesc gatedesc;
typedef struct trapframe trapframe;
typedef struct tss tss;
typedef enum procstate procstate;
typedef struct proc proc;
typedef struct context context;
typedef struct stackframe stackframe;
typedef struct e820_entry e820_entry;
typedef struct page page;

// serial.c
void serial_init(void);
int serial_getc(void);
void serial_putc(char c);
// seg.c
void seginit(void);
// trap.c
void trapinit(void);
// printk.c
int printk(const char *fmt, ...);
// page.c
void pageinit1(ulong end);
page *kalloc(void);
void kfree(page *p);
