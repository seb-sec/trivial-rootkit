/*
 *	simple kernel module
 */

#include <linux/module.h>
#include <linux/kernel.h>

/* function called on module initialisation */
int init_module(void) {
	printk(KERN_ALERT "Hello kernel\n");
	return 0;
}

void cleanup_module(void) {
	printk(KERN_ALERT "Goodbye kernel\n");
}