#include "user.h"

typedef enum tktype tktype;
typedef enum cmdtype cmdtype;
typedef struct proc proc;
typedef struct cmd cmd;
typedef struct stream stream;

enum tktype {
  TK_NORMAL,
  TK_PIPE,
  TK_REDIR,
  TK_REDIRA,
  TK_REDIRIN,
};

enum cmdtype {
  CMD_EXEC,
  CMD_PIPE,
  CMD_REDIR,
};

#define NPROC   8
#define NCMD    16
#define MAXARG  16
#define MAXTOK  128
#define MAXBUF  256

#define O_RDONLY  0
#define O_WRONLY  1
#define O_APPEND  (1 << 3)
#define O_CREAT   (1 << 4)

struct proc {
  bool used;
  char *args[MAXARG];
  int infd;
  int outfd;
  int pid;
  proc *next;
};

struct cmd {
  bool used;
  cmdtype type;
  union {
    struct {
      char *args[MAXARG];
    } e;
    struct {
      cmd *l;
      cmd *r;
    } p;
    struct {
      cmd *c;
      int fd;
      bool in;
    } r;
  };
};

struct stream {
  char *token[MAXTOK];
  int type[MAXTOK];
  int c;
  int n;
};

static inline int pipe(int *fd) { fd[0] = fd[1] = -1; return -1; }
static inline int dup2(int a, int b) { return -1; }

static proc procpool[NPROC];
static cmd cmdpool[NCMD];
static proc *proclist;

static proc *
allocproc(void)
{
  for (int i = 0; i < NPROC; i++) {
    if (!procpool[i].used) {
      memset(&procpool[i], 0, sizeof(proc));
      procpool[i].used = true;
      procpool[i].infd = 0;
      procpool[i].outfd = 1;
      return &procpool[i];
    }
  }
  return NULL;
}

static cmd *
alloccmd(cmdtype t)
{
  for (int i = 0; i < NCMD; i++) {
    if (!cmdpool[i].used) {
      memset(&cmdpool[i], 0, sizeof(cmd));
      cmdpool[i].used = true;
      cmdpool[i].type = t;
      return &cmdpool[i];
    }
  }
  return NULL;
}

static void
freeall(void)
{
  for (int i = 0; i < NPROC; i++)
    procpool[i].used = false;
  for (int i = 0; i < NCMD; i++)
    cmdpool[i].used = false;
  proclist = NULL;
}

static void
addproc(proc *p)
{
  proc *q;
  if (!proclist) {
    proclist = p;
    return;
  }
  for (q = proclist; q->next; q = q->next)
    ;
  q->next = p;
}

static char *
eat(stream *s)
{
  if (s->c >= s->n)
    return NULL;
  return s->token[s->c++];
}

static int
peektype(stream *s)
{
  if (s->c >= s->n)
    return -1;
  return s->type[s->c];
}

static cmd *
parseexec(stream *s)
{
  cmd *c;
  char *arg;
  int n = 0;
  c = alloccmd(CMD_EXEC);
  if (!c)
    return NULL;
  for (;;) {
    if (peektype(s) != TK_NORMAL)
      break;
    arg = eat(s);
    if (!arg)
      break;
    if (n >= MAXARG - 1)
      break;
    c->e.args[n++] = arg;
  }
  if (n == 0)
    return NULL;
  if (strcmp(c->e.args[0], "exit") == 0)
    exit(0);
  return c;
}

static cmd *
parsepipe(stream *s, cmd *left)
{
  cmd *c, *r;
  r = parseexec(s);
  if (!r)
    return NULL;
  c = alloccmd(CMD_PIPE);
  if (!c)
    return NULL;
  c->p.l = left;
  c->p.r = r;
  return c;
}

static cmd *
parseredir(stream *s, cmd *left, bool append, bool in)
{
  cmd *c;
  char *name;
  int fd;
  int flags;
  name = eat(s);
  if (!name) {
    printf("no file name\n");
    return NULL;
  }
  if (in)
    flags = O_RDONLY;
  else if (append)
    flags = O_WRONLY | O_APPEND | O_CREAT;
  else
    flags = O_WRONLY | O_CREAT;
  fd = open(name, flags);
  if (fd < 0) {
    printf("open failed: %s\n", name);
    return NULL;
  }
  c = alloccmd(CMD_REDIR);
  if (!c) {
    close(fd);
    return NULL;
  }
  c->r.c = left;
  c->r.fd = fd;
  c->r.in = in;
  return c;
}

static cmd *
parsepiperedir(stream *s)
{
  cmd *l;
  int d;
  l = parseexec(s);
  if (!l)
    return NULL;
  for (;;) {
    d = peektype(s);
    switch (d) {
      case TK_PIPE:
        eat(s);
        l = parsepipe(s, l);
        break;
      case TK_REDIR:
        eat(s);
        l = parseredir(s, l, false, false);
        break;
      case TK_REDIRA:
        eat(s);
        l = parseredir(s, l, true, false);
        break;
      case TK_REDIRIN:
        eat(s);
        l = parseredir(s, l, false, true);
        break;
      default:
        return l;
    }
    if (!l)
      return NULL;
  }
}

static proc *
parsecmd(cmd *c)
{
  proc *p;
  if (!c)
    return NULL;
  switch (c->type) {
    case CMD_EXEC:
      p = allocproc();
      if (!p)
        return NULL;
      for (int i = 0; i < MAXARG; i++)
        p->args[i] = c->e.args[i];
      addproc(p);
      return p;
    case CMD_PIPE:
      if (!parsecmd(c->p.l))
        return NULL;
      return parsecmd(c->p.r);
    case CMD_REDIR:
      p = parsecmd(c->r.c);
      if (!p)
        return NULL;
      if (c->r.in)
        p->infd = c->r.fd;
      else
        p->outfd = c->r.fd;
      return p;
  }
  return NULL;
}

static void
tokenize(char *buf, stream *s)
{
  static const char *ws = " \n\t";
  char *tok, *c;
  int n = 0;
  memset(s, 0, sizeof *s);
  if (!buf)
    return;
  tok = strtok(buf, ws);
  while (tok && n < MAXTOK) {
    s->token[n++] = tok;
    tok = strtok(NULL, ws);
  }
  for (int i = 0; i < n; i++) {
    c = s->token[i];
    if (strcmp(c, "|") == 0)
      s->type[i] = TK_PIPE;
    else if (strcmp(c, ">") == 0)
      s->type[i] = TK_REDIR;
    else if (strcmp(c, ">>") == 0)
      s->type[i] = TK_REDIRA;
    else if (strcmp(c, "<") == 0)
      s->type[i] = TK_REDIRIN;
    else
      s->type[i] = TK_NORMAL;
  }
  s->c = 0;
  s->n = n;
}

static void
runproc(proc *p)
{
  if (p->infd != 0) {
    dup2(p->infd, 0);
    close(p->infd);
  }
  if (p->outfd != 1) {
    dup2(p->outfd, 1);
    close(p->outfd);
  }
  exec(p->args[0], (const char**)p->args);
  printf("exec failed: %s\n", p->args[0]);
  exit(1);
}

static void
execjob(void)
{
  proc *p;
  int fd[2];
  int pid;
  for (p = proclist; p; p = p->next) {
    if (p->next) {
      pipe(fd);
      p->outfd = fd[1];
      p->next->infd = fd[0];
    }
    pid = fork();
    if (pid == 0)
      runproc(p);
    p->pid = pid;
    if (p->infd != 0)
      close(p->infd);
    if (p->outfd != 1)
      close(p->outfd);
  }
}

static void
waitjob(void)
{
  proc *p;
  int status;
  for (p = proclist; p; p = p->next)
    wait(&status);
}

static char *
readcmd(char *buf)
{
  int r;
  memset(buf, 0, MAXBUF);
  r = read(0, buf, MAXBUF);
  if (r <= 0)
    return NULL;
  return buf;
}

int
main(void)
{
  stream st;
  cmd *c;
  char buf[MAXBUF];
  char *s;
  for (;;) {
    printf("$ ");
    s = readcmd(buf);
    if (!s) {
      freeall();
      continue;
    }
    tokenize(s, &st);
    if (st.n == 0) {
      freeall();
      continue;
    }
    c = parsepiperedir(&st);
    if (!c) {
      freeall();
      continue;
    }
    if (!parsecmd(c)) {
      freeall();
      continue;
    }
    execjob();
    waitjob();
    freeall();
  }
}
