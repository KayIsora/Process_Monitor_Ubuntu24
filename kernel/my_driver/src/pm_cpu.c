#include <linux/smp.h>
#include <linux/random.h>
#include "pm_drv.h"

DEFINE_PER_CPU(struct pm_cpu_snap, pm_cpu_buf);

void pm_update_fake(void)
{
    int cpu;
    write_seqlock(&pm_seq);
    for_each_online_cpu(cpu) {
        struct pm_cpu_snap *s = per_cpu_ptr(&pm_cpu_buf, cpu);
        s->usage = (get_random_u32() % 100);
        s->ts = ktime_get_boottime_ns();
    }
    write_sequnlock(&pm_seq);
}
