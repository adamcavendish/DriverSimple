/**
 * hello-2.c -- Demostrating the module_init() and module_exit() macros.
 * This is preferred over using init_module() and cleanup_module().
 */
#include <linux/module.h>	// needed by all modules
#include <linux/kernel.h>	// needed by KERN_INFO
#include <linux/init.h>		// needed for the macros

static int __init hello_2_init(void) {
	printk(KERN_INFO "Hello, World 2\n");
	return 0;
}

static void __exit hello_2_exit(void) {
	printk(KERN_INFO "Goodbye, World! 2\n");
}

module_init(hello_2_init);
module_exit(hello_2_exit);

