#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include "pm_drv.h"

static struct proc_dir_entry *root;

static ssize_t cfg_write(struct file *f, const char __user *ubuf, size_t len, loff_t *off)
{
    char buf[64];
    unsigned int new_interval = pm_interval_ms;

    if (len >= sizeof(buf)) return -EINVAL;
    if (copy_from_user(buf, ubuf, len)) return -EFAULT;
    buf[len] = '\0';

    /* cú pháp: "1 [interval_ms]" hoặc "0" */
    if (buf[0] == '1') {
        if (sscanf(buf, "1 %u", &new_interval) == 1 && new_interval > 0)
            pm_interval_ms = new_interval;
        if (!atomic_xchg(&pm_enabled, 1)) {
            schedule_delayed_work(&pm_work, 0); /* bật tick ngay */
        }
    } else if (buf[0] == '0') {
        atomic_set(&pm_enabled, 0);
        cancel_delayed_work_sync(&pm_work);
    }
    return len;
}

static int stat_show(struct seq_file *m, void *v)
{
    unsigned seq, cpu;
    do {
        seq = read_seqbegin(&pm_seq);
        for_each_online_cpu(cpu) {
            const struct pm_cpu_snap *s = per_cpu_ptr(&pm_cpu_buf, cpu);
            seq_printf(m, "cpu%u.usage=%llu ts=%llu\n", cpu, s->usage, s->ts);
        }
    } while (read_seqretry(&pm_seq, seq));

    seq_printf(m, "enabled=%d interval_ms=%u\n", atomic_read(&pm_enabled), pm_interval_ms);
    return 0;
}

static int stat_open(struct inode *inode, struct file *file)
{
    return single_open(file, stat_show, NULL);
}

static const struct proc_ops cfg_fops = {
    .proc_write = cfg_write,
};
static const struct proc_ops stat_fops = {
    .proc_open    = stat_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* === Public init/exit để pm_core.c gọi === */
int pmfs_init(void)
{
    root = proc_mkdir("pm_drv", NULL);
    if (!root) return -ENOMEM;
    if (!proc_create("config", 0220, root, &cfg_fops)) return -ENOMEM;
    if (!proc_create("status", 0444, root, &stat_fops)) return -ENOMEM;
    return 0;
}

void pmfs_exit(void)
{
    if (root) {
        /* gỡ cả subtree để tránh rò rỉ entry */
        remove_proc_subtree("pm_drv", NULL);
        root = NULL;
    }
}
