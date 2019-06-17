/*
 * head for device file operation functions
 */

#include <linux/uaccess.h>		/* required to include <asm/uaccess.h> */
#include <asm/uaccess.h>		/* for handling data to userland buffers */
#include <linux/fs.h>			/* used for filesystem stuff */

static int trivial_open(struct inode *, struct file *);
static int trivial_close(struct inode *, struct file *);
static ssize_t trivial_write(struct file *, const char *, size_t, loff_t *);
static ssize_t trivial_read(struct file *, char *, size_t, loff_t *);

