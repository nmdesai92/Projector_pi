#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x425785a6, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x48f13226, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0xb8faa166, __VMLINUX_SYMBOL_STR(param_ops_bool) },
	{ 0x9323a91e, __VMLINUX_SYMBOL_STR(lirc_dev_fop_close) },
	{ 0x1ec5cd9b, __VMLINUX_SYMBOL_STR(lirc_dev_fop_open) },
	{ 0x51c762c1, __VMLINUX_SYMBOL_STR(lirc_dev_fop_poll) },
	{ 0x5ba66286, __VMLINUX_SYMBOL_STR(lirc_dev_fop_read) },
	{ 0x3a98abb5, __VMLINUX_SYMBOL_STR(no_llseek) },
	{ 0xfe990052, __VMLINUX_SYMBOL_STR(gpio_free) },
	{ 0x4dca738, __VMLINUX_SYMBOL_STR(lirc_unregister_driver) },
	{ 0x26332062, __VMLINUX_SYMBOL_STR(lirc_register_driver) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0x9247f4b, __VMLINUX_SYMBOL_STR(of_property_read_variable_u32_array) },
	{ 0xe9e36888, __VMLINUX_SYMBOL_STR(of_property_read_u32_index) },
	{ 0xf1efbee0, __VMLINUX_SYMBOL_STR(of_parse_phandle) },
	{ 0xdb6fa696, __VMLINUX_SYMBOL_STR(gpiochip_find) },
	{ 0x2de4a971, __VMLINUX_SYMBOL_STR(platform_device_put) },
	{ 0x390aae, __VMLINUX_SYMBOL_STR(platform_device_add) },
	{ 0x59455b6c, __VMLINUX_SYMBOL_STR(platform_device_alloc) },
	{ 0xbb37e3ff, __VMLINUX_SYMBOL_STR(of_node_put) },
	{ 0x16305289, __VMLINUX_SYMBOL_STR(warn_slowpath_null) },
	{ 0x8ae08255, __VMLINUX_SYMBOL_STR(of_find_device_by_node) },
	{ 0xbfda7a8b, __VMLINUX_SYMBOL_STR(of_find_compatible_node) },
	{ 0x3d7568f5, __VMLINUX_SYMBOL_STR(__platform_driver_register) },
	{ 0xc068440e, __VMLINUX_SYMBOL_STR(__kfifo_alloc) },
	{ 0x275ef902, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0xd6b8e852, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0x1e047854, __VMLINUX_SYMBOL_STR(warn_slowpath_fmt) },
	{ 0xdb760f52, __VMLINUX_SYMBOL_STR(__kfifo_free) },
	{ 0xea2219bc, __VMLINUX_SYMBOL_STR(platform_driver_unregister) },
	{ 0x3709785e, __VMLINUX_SYMBOL_STR(platform_device_unregister) },
	{ 0xf20dabd8, __VMLINUX_SYMBOL_STR(free_irq) },
	{ 0x3ce4ca6f, __VMLINUX_SYMBOL_STR(disable_irq) },
	{ 0xbc477a2, __VMLINUX_SYMBOL_STR(irq_set_irq_type) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xe2e8065e, __VMLINUX_SYMBOL_STR(memdup_user) },
	{ 0x94eea794, __VMLINUX_SYMBOL_STR(getnstimeofday64) },
	{ 0xd85cd67e, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0x4f68e5c9, __VMLINUX_SYMBOL_STR(do_gettimeofday) },
	{ 0xf23fcb99, __VMLINUX_SYMBOL_STR(__kfifo_in) },
	{ 0x51d559d1, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irqrestore) },
	{ 0x598542b2, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irqsave) },
	{ 0x305f7161, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x19d3b7d8, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0x45b7c981, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0xb1ea493e, __VMLINUX_SYMBOL_STR(sysfs_remove_group) },
	{ 0x3060ab59, __VMLINUX_SYMBOL_STR(lirc_dev_fop_ioctl) },
	{ 0x353e3fa5, __VMLINUX_SYMBOL_STR(__get_user_4) },
	{ 0x8e865d3c, __VMLINUX_SYMBOL_STR(arm_delay_ops) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
	{ 0xe707d823, __VMLINUX_SYMBOL_STR(__aeabi_uidiv) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=lirc_dev";

MODULE_ALIAS("of:N*T*Crpi,lirc-rpi");
MODULE_ALIAS("of:N*T*Crpi,lirc-rpiC*");

MODULE_INFO(srcversion, "4A9EC61273DEA04FF427BE9");
