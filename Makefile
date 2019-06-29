
# Module
obj-m 				:= not_a_rootkit.o

not_a_rootkit-y		+= src/not_a_rootkit.o
not_a_rootkit-y		+= src/hooking.o
not_a_rootkit-y		+= src/priv_escalation.o
not_a_rootkit-y		+= src/module_hide.o

ccflags-y			:= -I$(PWD)/src -I$(PWD)/src/incl

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean