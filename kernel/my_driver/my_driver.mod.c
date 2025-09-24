#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0x534ed5f3, "__msecs_to_jiffies" },
	{ 0xf8d7ac5e, "proc_create" },
	{ 0x388dee05, "seq_lseek" },
	{ 0x1f678830, "remove_proc_subtree" },
	{ 0x5ae9ee26, "__per_cpu_offset" },
	{ 0xde338d9a, "_raw_spin_lock" },
	{ 0xd272d446, "__fentry__" },
	{ 0xe8213e80, "_printk" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x8ce83585, "queue_delayed_work_on" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0x86632fd6, "_find_next_bit" },
	{ 0xb5c51982, "__cpu_online_mask" },
	{ 0x173ec8da, "sscanf" },
	{ 0xda212183, "proc_mkdir" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xf296206e, "nr_cpu_ids" },
	{ 0xd22cd56f, "seq_read" },
	{ 0x85acaba2, "cancel_delayed_work_sync" },
	{ 0x02f9bbf0, "init_timer_key" },
	{ 0x12cfb334, "seq_printf" },
	{ 0xc01aafd2, "get_random_u32" },
	{ 0x71798f7e, "delayed_work_timer_fn" },
	{ 0xae030cd0, "single_release" },
	{ 0x5218fe90, "single_open" },
	{ 0xde338d9a, "_raw_spin_unlock" },
	{ 0x12ca6142, "ktime_get_with_offset" },
	{ 0xaef1f20d, "system_wq" },
	{ 0x70eca2ca, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xa61fd7aa,
	0x092a35a2,
	0x534ed5f3,
	0xf8d7ac5e,
	0x388dee05,
	0x1f678830,
	0x5ae9ee26,
	0xde338d9a,
	0xd272d446,
	0xe8213e80,
	0xd272d446,
	0x8ce83585,
	0x90a48d82,
	0x86632fd6,
	0xb5c51982,
	0x173ec8da,
	0xda212183,
	0xd272d446,
	0xf296206e,
	0xd22cd56f,
	0x85acaba2,
	0x02f9bbf0,
	0x12cfb334,
	0xc01aafd2,
	0x71798f7e,
	0xae030cd0,
	0x5218fe90,
	0xde338d9a,
	0x12ca6142,
	0xaef1f20d,
	0x70eca2ca,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"__check_object_size\0"
	"_copy_from_user\0"
	"__msecs_to_jiffies\0"
	"proc_create\0"
	"seq_lseek\0"
	"remove_proc_subtree\0"
	"__per_cpu_offset\0"
	"_raw_spin_lock\0"
	"__fentry__\0"
	"_printk\0"
	"__stack_chk_fail\0"
	"queue_delayed_work_on\0"
	"__ubsan_handle_out_of_bounds\0"
	"_find_next_bit\0"
	"__cpu_online_mask\0"
	"sscanf\0"
	"proc_mkdir\0"
	"__x86_return_thunk\0"
	"nr_cpu_ids\0"
	"seq_read\0"
	"cancel_delayed_work_sync\0"
	"init_timer_key\0"
	"seq_printf\0"
	"get_random_u32\0"
	"delayed_work_timer_fn\0"
	"single_release\0"
	"single_open\0"
	"_raw_spin_unlock\0"
	"ktime_get_with_offset\0"
	"system_wq\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "0364D72D1E3893980DACABC");
