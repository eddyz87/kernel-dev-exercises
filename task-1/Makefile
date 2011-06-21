obj-m := t1-main.o
#module-objs := t1-main.c

KERNEL_DIR=/lib/modules/`uname -r`/build

default:
	make -C $(KERNEL_DIR) M=`pwd` modules

clean:
	@rm -f $(obj-m:.o=.mod.c) $(obj-m:.o=.mod.o) $(obj-m:.o=.ko) $(obj-m) modules.order  Module.symvers
