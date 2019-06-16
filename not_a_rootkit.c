/*
 *	simple kernel module
 */

#include <linux/module.h>		/* used by all modules */
#include <linux/kernel.h>		/* used by KERN_INFO and others */
#include <linux/init.h>			/* used for macros */
#include <linux/fs.h>			/* used for filesystem stuff */
#include <linux/uaccess.h>		/* required to include <asm/uaccess.h> */
#include <asm/uaccess.h>		/* for handling data to userland buffers */


MODULE_LICENSE("GPL");
MODULE_AUTHOR("me");
MODULE_DESCRIPTION("simple module");


/* prototypes */
int __init innocent_init(void);
void __exit innocent_exit(void);
static int trivial_open(struct inode *, struct file *);
static int trivial_close(struct inode *, struct file *);
static ssize_t trivial_write(struct file *, const char *, size_t, loff_t *);
static ssize_t trivial_read(struct file *, char *, size_t, loff_t *);


/* useful defines */
#define DEVICE_NAME "not_a_rootkit"		// name appears in /proc/devices


/* globals (within this file) */
static int majorNum;					// device major number assigned
static int deviceOpen = 0;				// track if device open, may be useful later

/* used to store any data aquired, given back during a read()- needs to be allocated */
static char *deviceBuf = NULL;			

/* must declare file system operations that device can perform- see linux/fs.h */
static struct file_operations fops = {
	.read = trivial_read,
	.write = trivial_write,
	.open = trivial_open,
	.release = trivial_close
};

/*---------------- startup/shutdown ------------------------*/
/* function called on module initialisation */
int __init trivial_init(void) {

	/* register our device with the kernel */
	majorNum = register_chrdev(0, DEVICE_NAME, &fops);

	if (majorNum < 0) {
		printk(KERN_ALERT "Failed to register rootkit chardevice\n");
		return majorNum;
	}
	printk(KERN_INFO "Registered rootkit with the kernel. Thanks kernel\n");

	/* probably a whole lot of stealth work here */

	return 0;
}

/* function called on module removal, should reverse init function */
void __exit trivial_exit(void) {
	
	/* unregister the device, cover our traces */
	unregister_chrdev(majorNum, DEVICE_NAME);
}


/*-------------------- functions --------------------------*/

/* called when device is opened- required to use other functions, so just return success */
static int trivial_open(struct inode *inode, struct file *f) {
	printk(KERN_INFO "Inconspicuous device opened\n");
	return 0;
}

/* not sure if this is required, may have some use for this in future though */
static int trivial_close(struct inode *inode, struct file *f) {
	return 0;
}

/* 
 * called when a process writes to the device, used to execute specific commands 
 * expects the amount of bytes written returned
 */
static ssize_t trivial_write(struct file *f, const char *buff, size_t len, loff_t *off) {
	printk(KERN_INFO "Write performed to inconspicuous device\n");
	return len;
}

/*
 * called when a process reads from the device, expects number of bytes read to be returned
 * maybe we give back custom data (say, from a command given to write()) in future
 */
static ssize_t trivial_read(struct file *f, char *buff, size_t length, loff_t *offset) {
	printk(KERN_INFO "Read performed on inconspicuous device\n");
	return length;
}


/* must call these and pass in our init/exit functions */
module_init(trivial_init);
module_exit(trivial_exit);