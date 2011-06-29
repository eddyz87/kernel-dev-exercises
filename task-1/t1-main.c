#include <linux/init.h>
#include <linux/module.h>

#include "log.h"
#include "resource_manager.h"
#include "params.h"
#include "procfs.h"

MODULE_LICENSE("GPL");

int t1_that_init_result = 0;
int t1_do_stupid_things = 0;
char t1_stupid_thing_kind[MAX_CHARP_LENGTH] = "divide-by-zero";  //makeing this char[] only for working with sysctl

module_param(t1_that_init_result, int, S_IRUGO | S_IWUSR);
module_param(t1_do_stupid_things, int, S_IRUGO | S_IWUSR);
// what is name needed for?
module_param_string(t1_stupid_thing_kind, t1_stupid_thing_kind, sizeof(t1_stupid_thing_kind), S_IRUGO | S_IWUSR);

/*

Q: What type of locking is needed when I work with variables that could be changed from procfs ?

*/

int do_stupid_thing(void) {
    if (t1_do_stupid_things) {
        printk(KERN_ALERT ">>> gona do stupid thing\n");
        if (strncmp(t1_stupid_thing_kind, "divide-by-zero", sizeof(t1_stupid_thing_kind)) == 0) {
            int i = 0;
            return 10/i;
        }
        if (strncmp(t1_stupid_thing_kind, "call-zero", sizeof(t1_stupid_thing_kind)) == 0) {
            return ((int (*) ()) 0)();
        }
        printk(KERN_ALERT "<<< stupid thing is done, nobody would see this if params are correct\n");
    }
    return 0;
}

static int take_this(void) {
    LOG(LC_THIS, 0, "--- this is taken\n");
    do_stupid_thing();
    return 0;
}

static void leave_this(void) {
    LOG(LC_THIS, 0, "--- this is leaved\n");
    do_stupid_thing();
}

static int take_that(void) {
    LOG(LC_THAT, 0, "------ that is taken\n");
    return t1_that_init_result;
}

static void leave_that(void) {
    LOG(LC_THAT, 0, "------ that is leaved\n");
}

struct t1_resource my_resources[] = {
    RC_INIT(register_log_channels, unregister_log_channels),
    RC_INIT(take_this, leave_this),
    RC_INIT(take_that, leave_that),
    RC_INIT(t1_procfs_register, t1_procfs_unregister),
};

static int __init t1_init(void) {
    printk(KERN_ALERT "Module t1 is being loaded\n");
    if (t1_take_resources(my_resources, ARRAY_SIZE(my_resources)) < 0) {
        printk(KERN_ALERT "Module t1 can't initialize\n");
        return -EFAULT;
    }
    LOG(LC_MAIN, 0, "Module t1 init ok\n");
    return 0;
}

static void __exit t1_exit(void) {
    LOG(LC_MAIN, 0, "Module t1 is being unloaded\n");
    t1_release_resources(my_resources, ARRAY_SIZE(my_resources));
    LOG(LC_MAIN, 0, "Module t1 unload is finished\n");
}

module_init(t1_init);
module_exit(t1_exit);
