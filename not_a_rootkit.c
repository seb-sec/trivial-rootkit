/*
 *	simple kernel module
 */

#include <linux/module.h>		/* used by all modules */
#include <linux/kernel.h>		/* used by KERN_INFO and others */
#include <linux/init.h>			/* used for macros */
#include <linux/fs.h>			/* used for filesystem stuff */
#include <linux/uaccess.h>		/* required to include <asm/uaccess.h> */
#include <asm/uaccess.h>		/* for handling data to userland buffers */
#include <linux/device.h>		/* for class/device creation */
#include <linux/slab.h>			/* for kernel memory allocation */
#include <linux/string.h>		/* for string functions */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("me");
MODULE_DESCRIPTION("simple module");
MODULE_VERSION("0.0.1");

/* prototypes */
int __init trivial_init(void);
void __exit trivial_exit(void);
static int trivial_open(struct inode *, struct file *);
static int trivial_close(struct inode *, struct file *);
static ssize_t trivial_write(struct file *, const char *, size_t, loff_t *);
static ssize_t trivial_read(struct file *, char *, size_t, loff_t *);


/* useful defines */
#define DEVICE_NAME "not_a_rootkit"		// name appears in /proc/devices
#define CLASS_NAME 	"not_a"
#define SUCCESS		0

/* globals (within this file) */
static int majorNum;					// device major number assigned
static int deviceOpen = 0;				// track if device open, may be useful later
static struct class *devClass = NULL;
static struct device *devStruct = NULL;

/* used to store any data aquired, given back during a read()- needs to be allocated */
static char *devBuf = NULL;			

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
	
	/* register the device class */
	devClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(devClass)) {
		unregister_chrdev(majorNum, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register rootkit char class\n");
		return -1;		// tmp value
	}

	/* create the device */
	devStruct = device_create(devClass, NULL, MKDEV(majorNum, 0), NULL, DEVICE_NAME);
	if (IS_ERR(devStruct)) {
		class_destroy(devClass);
		unregister_chrdev(majorNum, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create rootkit device\n");
		return -1;		// tmp value
	}

	printk(KERN_INFO "Registered rootkit with the kernel. Thanks kernel\n");
	/* probably a whole lot of stealth work here */

	return 0;
}

/* function called on module removal, should reverse init function */
void __exit trivial_exit(void) {
	
	/* unregister the device, cover our traces */
	device_destroy(devClass, MKDEV(majorNum, 0));
	class_unregister(devClass);
	class_destroy(devClass);
	unregister_chrdev(majorNum, DEVICE_NAME);
}

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
	//printk(KERN_INFO "Write performed to inconspicuous device\n");

	char *privBuf;				// our own buffer in kernel space
	char magic[] = "do-";		// magic prefix for all commands
	char *cmdPtr;				// track where in command statement we are later

	privBuf = (char *) kmalloc(len+1, GFP_KERNEL);	// allocate normal kernel RAM
	if (privBuf == NULL) {
		printk(KERN_ALERT "ROOTKIT: failed to allocate kernel buffer");
		return len;
	}

	/* we are working with a user buffer, must copy into kernel space */
	copy_from_user(privBuf, buff, len);

	/* if the string starts with the magic prefix, we try to execute the command */
	if (memcmp(privBuf, magic, 3) == SUCCESS) {
		cmdPtr = &buff[3];			// cmdPtr should now be the given command

		/* TODO: place our if/elseif strcmp tower here for future commands */
		printk(KERN_INFO "ROOTKIT: You issued the command: %s\n", cmdPtr);

	}
	kfree(privBuf);		// leave no trace
	return len;
}

/*
 * called when a process reads from the device, expects number of bytes read to be returned
 * maybe we give back custom data (say, from a command given to write()) in future
 */
static ssize_t trivial_read(struct file *f, char *buff, size_t length, loff_t *offset) {
	printk(KERN_INFO "Read performed on inconspicuous device\n");
	return 0;		// signal EOF for now
}

/* must call these and pass in our init/exit functions */
module_init(trivial_init);
module_exit(trivial_exit);