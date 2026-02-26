#ifndef _TIMER_H
#define _TIMER_H

#include <device.h>
#include <irq.h>
#include <kernel.h>

struct timer;
struct eventtimer;

struct timer_if {
  ulong (*usec2period)(struct timer *tm, uint usec);
  ulong (*read)(struct timer *tm);
};

struct timer {
  struct device dev;

  void *priv;
  bool global;

  struct timer_if *ops;
};

#define dev_timer(_d) container_of(_d, struct timer, dev)

struct eventtimer_if {
  uint (*getperiod)(struct eventtimer *et);
  void (*setperiod)(struct eventtimer *et, uint ms);
  int (*irqhandler)(struct eventtimer *et, struct irq *irq);
};

struct eventtimer {
  struct device dev;

  void *priv;
  bool global;

  struct eventtimer_if *ops;
};

#define dev_eventtimer(_d) container_of(_d, struct eventtimer, dev)

int eventtimerirq(struct irq *irq);

int probe_evtimer(struct device *dev);
int probe_timer(struct device *dev);
void msleep(uint msec);
void usleep(uint usec);

#endif // _TIMER_H
