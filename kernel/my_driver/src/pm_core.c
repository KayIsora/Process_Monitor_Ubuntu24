#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include "pm_drv.h"

seqlock_t pm_seq;
atomic_t pm_enabled = ATOMIC_INIT(0);
unsigned int pm_interval_ms = 500;

struct delayed_work pm_work;

static void pm_tick(struct work_struct* ws)
{
    if (atomic_read(&pm_enabled)) {
        pm_update_fake();
        schedule_delayed_work(&pm_work, msecs_to_jiffies(pm_interval_ms));
    }
}

static int __init mydrv_init(void)
{
    int rc;
    seqlock_init(&pm_seq);
    INIT_DELAYED_WORK(&pm_work, pm_tick);

    rc = pmfs_init();
    if (rc) {
        pr_err("my_driver: pmfs_init failed %d\n", rc);
        return rc;
    }

    pr_info("my_driver: init\n");
    return 0;
}

static void __exit mydrv_exit(void)
{
    cancel_delayed_work_sync(&pm_work);
    pmfs_exit();
    pr_info("my_driver: exit\n");
}

module_init(mydrv_init);
module_exit(mydrv_exit);
MODULE_LICENSE("GPL");
