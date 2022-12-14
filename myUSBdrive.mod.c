#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

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

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xf704969, "module_layout" },
	{ 0xb7f634e0, "usb_deregister" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0x8f63b77a, "usb_register_driver" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x33659ce1, "usb_deregister_dev" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0xe7e96aee, "usb_put_dev" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xed77a7c6, "usb_register_dev" },
	{ 0xc4ae0b74, "usb_get_dev" },
	{ 0x7c797b6, "kmem_cache_alloc_trace" },
	{ 0xd731cdd9, "kmalloc_caches" },
	{ 0x296695f, "refcount_warn_saturate" },
	{ 0xa9617a3a, "usb_find_interface" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0xf4676653, "usb_bulk_msg" },
	{ 0x37a0cba, "kfree" },
	{ 0x41882bd9, "usb_free_urb" },
	{ 0x9a536a95, "usb_submit_urb" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xe1b5d8df, "usb_alloc_coherent" },
	{ 0x249a9ef3, "usb_alloc_urb" },
	{ 0x92997ed8, "_printk" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xea7b7d10, "usb_free_coherent" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v8564p1000d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v090Cp1000d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "6EBE9BC87E9C815EDC0DE16");
