obj-m := t2.o
t2-y := t2-main.o

OBJS=t2.o t2-main.o resource_manager.o procfs.o

KERNEL_DIR=/lib/modules/`uname -r`/build

EXTRA_CFLAGS+=-Wall

default:
	make -C $(KERNEL_DIR) M=`pwd` modules

clean:
	@rm -f $(OBJS:.o=.mod.c) $(OBJS:.o=.mod.o) $(OBJS:.o=.ko) $(OBJS) modules.order  Module.symvers
