#ifndef _MODULE_H
#define _MODULE_H

#include <kernel.h>
#include <elf.h>

typedef struct MODULE   MODULE;
typedef struct MODELF   MODELF;

struct MODULE
{
  const char      *name;
  const char      *description;
  MODELF          *elf;

  void            (*init) (void);
  void            (*delete) (void);

  bool            initialized;
  const char      *deps;
};

struct MODELF
{
       ; 
};

int moduleload (MODELF *elf);
void initmodule (void);

#define MODULE_DECL     \
  static USED SECTION (".module") ALIGNED (_Alignof (MODULE)) MODULE

#endif  // _MODULE_H
