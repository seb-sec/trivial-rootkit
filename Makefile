MODULE_NAME = not_a_rootkit

SRCS = 	src/not_a_rootkit.c \
		src/dev_fops.c

INCLUDES = -I$(src)/incl

ccflags-y := $(INCLUDES) 	

OBJS = $(SRCS:.c=.o)

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-y = $(OBJS)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean