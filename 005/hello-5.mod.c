#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0x8ca1999d, "module_layout" },
	{ 0xb2d307de, "param_ops_short" },
	{ 0x15692c87, "param_ops_int" },
	{ 0x4470a79b, "param_ops_long" },
	{ 0x35b6b772, "param_ops_charp" },
	{ 0x4845c423, "param_array_ops" },
	{ 0x27e1a049, "printk" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "327B75AD309D5EB7FB29FDD");
