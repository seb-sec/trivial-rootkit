
#ifndef _MOD_HIDE_H
#define _MOD_HIDE_H

#include <linux/module.h>		/* used by all modules */
#include <linux/kernel.h>		/* used by KERN_INFO and others */

extern struct list_head list_head_saved;
extern struct module_kobject mkobj_saved;
extern struct list_head *module_previous;

void hide_module(int *hidden_flag);
void unhide_module(int *hidden_flag);

#endif /* _MOD_HIDE_H */