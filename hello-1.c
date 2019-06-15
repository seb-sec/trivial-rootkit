/*
 *	simple kernel module
 */

#include <linux/module.h>		/* used by all modules */
#include <linux/kernel.h>		/* used by KERN_INFO and others */
#include <linux/init.h>			/* used for macros */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("me");
MODULE_DESCRIPTION("simple module");

/* function called on module initialisation */
int __init hello_1_init(void) {
	printk(KERN_INFO "Hello kernel\n");		// find in /var/log/kern.log
	return 0;
}

/* function called on module removal, should reverse init function */
void __exit hello_1_exit(void) {
	printk(KERN_INFO "Goodbye kernel\n");
}

/* must call these and pass in our init/exit functions */
module_init(hello_1_init);
module_exit(hello_1_exit);