#pragma once
#include <linux/seqlock.h>
#include <linux/percpu.h>
#include <linux/ktime.h>
#include <linux/atomic.h>
#include <linux/workqueue.h>

struct pm_cpu_snap { u64 usage; u64 ts; };
DECLARE_PER_CPU(struct pm_cpu_snap, pm_cpu_buf);

extern seqlock_t pm_seq;
extern atomic_t pm_enabled;
extern unsigned int pm_interval_ms;

extern struct delayed_work pm_work;

void pm_update_fake(void);

/* add: procfs init/exit */
int pmfs_init(void);
void pmfs_exit(void);
