
#ifndef _PRIV_ESC_H
#define _PRIV_ESC_H

#include <asm/current.h>
#include <linux/cred.h>			/* credential structures */

struct task_struct *find_task_from_pid(int);
void root_creds(struct cred *);
void give_root(int);


#endif /* _PRIV_ESC_H */