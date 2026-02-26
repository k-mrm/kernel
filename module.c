#include <kernel.h>
#include <module.h>
#include <string.h>

#define KPREFIX "Module:"

#include <printk.h>

extern MODULE __module_s[];
extern MODULE __module_e[];

static int modelfcheck(void *p) { return -1; }

static int readmodelf(void *p, MODELF *elf) { return -1; }

int moduleload(MODELF *elf) { return -1; }

MODULE *findmodule(char *name) {
  for (MODULE *m = __module_s; m < __module_e; m++) {
    if (strcmp(m->name, name) == 0) {
      return m;
    }
  }

  return NULL;
}

static char *nextdepmod(char *deps, MODULE **dep) {
  char *dname = strtok(deps, ",");

  if (dname)
    *dep = findmodule(dname);
  return dname;
}

void delmodule(MODULE *m) {
  if (m && m->delete)
    m->delete ();
}

static int __modinit(MODULE *m) {
  MODULE *dep;
  char deps[100] = {0};

  if (m->initialized) {
    return 0;
  }

  if (m->deps) {
    strcpy(deps, m->deps);
    for (char *c = nextdepmod(deps, &dep); c; c = nextdepmod(NULL, &dep)) {
      if (!dep || dep == m || __modinit(dep) < 0) {
        warn("fail: %s\n", m->name);
        return -1;
      }
    }
  }

  log("Module Initializing: %s\n", m->name);

  if (m->init) {
    m->init();
  }
  m->initialized = true;
  return 0;
}

void initmodule(void) {
  for (MODULE *m = __module_s; m < __module_e; m++) {
    __modinit(m);
  }
}
