/*
 * Implementation of file operations on the device
 */
#include "../incl/dev_fops.h"
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
	return 0;		// signal EOF for now
}