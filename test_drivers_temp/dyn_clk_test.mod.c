#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

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

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x88e7265a, "module_layout" },
	{ 0xfe28924, "platform_driver_unregister" },
	{ 0x3f41c6, "__platform_driver_register" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xb6e6d99d, "clk_disable" },
	{ 0x76d9b876, "clk_set_rate" },
	{ 0xb742f5e, "_dev_info" },
	{ 0x556e4390, "clk_get_rate" },
	{ 0xb077e70a, "clk_unprepare" },
	{ 0x815588a6, "clk_enable" },
	{ 0x7c9a7371, "clk_prepare" },
	{ 0x2939a4dd, "_dev_err" },
	{ 0xe9fe1298, "devm_clk_get" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cdyn,dynclk-test");
MODULE_ALIAS("of:N*T*Cdyn,dynclk-testC*");
