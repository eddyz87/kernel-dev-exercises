obj-m := t1.o
t1-y := t1-main.o resource_manager.o procfs.o

OBJS=t1.o t1-main.o resource_manager.o procfs.o

KERNEL_DIR=/lib/modules/`uname -r`/build

default:
	make -C $(KERNEL_DIR) M=`pwd` modules

clean:
	@rm -f $(OBJS:.o=.mod.c) $(OBJS:.o=.mod.o) $(OBJS:.o=.ko) $(OBJS) modules.order  Module.symvers
