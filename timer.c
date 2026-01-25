#include <kernel.h>
#include <timer.h>
#include <panic.h>
#include <printk.h>
#include <proc.h>
#include <x86/cpu.h>

#define MSEC2USEC	1000

static struct timer *systimer;

static struct timer *
usingtimer(void)
{
        return systimer;
}

void
msleep(uint msec)
{
        uint usec = msec * MSEC2USEC;
        usleep(usec);
}

void
usleep(uint usec)
{
        struct timer *timer = usingtimer();
        ulong now, after;

        if (!timer)
                return;
        now = timer->ops->read(timer);
        after = now + timer->ops->usec2period(timer, usec);

        while (timer->ops->read(timer) < after)
                ;
}

int
eventtimerirq(struct irq *irq)
{
        struct device *dev = irq->device;
        struct eventtimer *et = dev_eventtimer(dev); 
        int ret;

        ret = et->ops->irqhandler(et, irq);

	// do process scheduling
        schedule();

        return ret;
}

int
probe_evtimer(struct device *dev)
{
	struct eventtimer *et = container_of(dev, struct eventtimer, dev);

        if (et->global) {
                ;
        } else {
                ;
        }
        return 0;
}

int
probe_timer(struct device *dev)
{
	struct timer *tm = container_of(dev, struct timer, dev);

        if (tm->global) {
                if (systimer)
                        warn("systimer changed\n");
                log("new systimer: %s\n", dev->name);
                systimer = tm;
                return 0;
        } else {
                return -1;
        }
}
