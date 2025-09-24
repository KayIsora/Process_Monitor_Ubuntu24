#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
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

#ifdef CONFIG_MITIGATION_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const char ____versions[]
__used __section("__versions") =
	"\x1c\x00\x00\x00\x48\x9f\xdb\x88"
	"__check_object_size\0"
	"\x18\x00\x00\x00\xc2\x9c\xc4\x13"
	"_copy_from_user\0"
	"\x1c\x00\x00\x00\x8f\x18\x02\x7f"
	"__msecs_to_jiffies\0\0"
	"\x14\x00\x00\x00\x2b\x3a\x21\x7f"
	"proc_create\0"
	"\x14\x00\x00\x00\xe5\x41\x2a\xae"
	"seq_lseek\0\0\0"
	"\x1c\x00\x00\x00\x53\x03\x11\x66"
	"remove_proc_subtree\0"
	"\x1c\x00\x00\x00\x53\x54\x9a\xb1"
	"__per_cpu_offset\0\0\0\0"
	"\x18\x00\x00\x00\x64\xbd\x8f\xba"
	"_raw_spin_lock\0\0"
	"\x14\x00\x00\x00\xbb\x6d\xfb\xbd"
	"__fentry__\0\0"
	"\x10\x00\x00\x00\x7e\x3a\x2c\x12"
	"_printk\0"
	"\x1c\x00\x00\x00\xcb\xf6\xfd\xf0"
	"__stack_chk_fail\0\0\0\0"
	"\x20\x00\x00\x00\x6d\xb5\xfc\xb2"
	"queue_delayed_work_on\0\0\0"
	"\x28\x00\x00\x00\xb3\x1c\xa2\x87"
	"__ubsan_handle_out_of_bounds\0\0\0\0"
	"\x18\x00\x00\x00\xd9\xe8\xa1\x53"
	"_find_next_bit\0\0"
	"\x1c\x00\x00\x00\x71\x22\x5a\x5a"
	"__cpu_online_mask\0\0\0"
	"\x10\x00\x00\x00\xe6\x6e\xab\xbc"
	"sscanf\0\0"
	"\x14\x00\x00\x00\x63\xec\x3d\xbc"
	"proc_mkdir\0\0"
	"\x1c\x00\x00\x00\xca\x39\x82\x5b"
	"__x86_return_thunk\0\0"
	"\x14\x00\x00\x00\xd5\xe3\x7d\x01"
	"nr_cpu_ids\0\0"
	"\x14\x00\x00\x00\x0e\xd8\xd5\xd9"
	"seq_read\0\0\0\0"
	"\x24\x00\x00\x00\x4a\x18\xa7\x9f"
	"cancel_delayed_work_sync\0\0\0\0"
	"\x18\x00\x00\x00\x39\x63\xf4\xc6"
	"init_timer_key\0\0"
	"\x14\x00\x00\x00\x69\xb6\x98\xa4"
	"seq_printf\0\0"
	"\x18\x00\x00\x00\x0c\xc1\x6d\xd3"
	"get_random_u32\0\0"
	"\x20\x00\x00\x00\x6a\xdf\xee\xff"
	"delayed_work_timer_fn\0\0\0"
	"\x18\x00\x00\x00\x0a\x8d\x36\x7f"
	"single_release\0\0"
	"\x14\x00\x00\x00\xb7\x6f\x70\xe6"
	"single_open\0"
	"\x1c\x00\x00\x00\x34\x4b\xb5\xb5"
	"_raw_spin_unlock\0\0\0\0"
	"\x20\x00\x00\x00\x12\xda\xf0\xc4"
	"ktime_get_with_offset\0\0\0"
	"\x14\x00\x00\x00\xd3\x85\x33\x2d"
	"system_wq\0\0\0"
	"\x18\x00\x00\x00\xde\x9f\x8a\x25"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "0364D72D1E3893980DACABC");
