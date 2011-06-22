#include <linux/init.h>
#include <linux/module.h>

#include "resource_manager.h"

MODULE_LICENSE("GPL");

#define MAX_CHARP_LENGTH    1024    // this magic number is typed directly into param_set_charp

static int that_init_result = 0;
static int do_stupid_things = 0;
static char* stupid_thing_kind = "divide-by-zero";

module_param(that_init_result, int, S_IRUGO | S_IWUSR);
module_param(do_stupid_things, int, S_IRUGO | S_IWUSR);
module_param(stupid_thing_kind, charp, S_IRUGO | S_IWUSR);

/*

Q: When I load this modeule with do_stupid_things=1 it fails at init with smth wierd printing stacktrace.
   lsmod shows that the module is loaded and that it's usage count is 1 but does not show who uses the module.
   I suspect that usage count is inc'ed/dec'ed by loader, have to check this.

*/

int do_stupid_thing(void) {
    if (do_stupid_things) {
        printk(KERN_ALERT ">>> gona do stupid thing\n");
        if (strncmp(stupid_thing_kind, "divide-by-zero", MAX_CHARP_LENGTH) == 0) {
            int i = 0;
            return 10/i;
        }
        if (strncmp(stupid_thing_kind, "call-zero", MAX_CHARP_LENGTH) == 0) {
            return ((int (*) ()) 0)();
        }
        printk(KERN_ALERT "<<< stupid thing is done, nobody would see this if params are correct\n");
    }
    return 0;
}

static int take_this(void) {
    printk(KERN_ALERT "--- this is taken\n");
    do_stupid_thing();
    return 0;
}

static void leave_this(void) {
    printk(KERN_ALERT "--- this is leaved\n");
    do_stupid_thing();
}

static int take_that(void) {
    printk(KERN_ALERT "------ that is taken\n");
    return that_init_result;
}

static void leave_that(void) {
    printk(KERN_ALERT "------ that is leaved\n");
}

struct t1_resource my_resources[2] = {
    RC_INIT(take_this, leave_this),
    RC_INIT(take_that, leave_that),
};

static int __init t1_init(void) {
    printk(KERN_ALERT "Module t1 is being loaded\n");
    if (t1_take_resources(my_resources, ARRAY_SIZE(my_resources)) < 0) {
        printk(KERN_ALERT "Module t1 can't initialize\n");
        return -1;
    }
    printk(KERN_ALERT "Module t1 init ok\n");
    return 0;
}

static void __exit t1_exit(void) {
    printk(KERN_ALERT "Module t1 is being unloaded\n");
    t1_release_resources(my_resources, ARRAY_SIZE(my_resources));
    printk(KERN_ALERT "Module t1 unload is finished\n");
}

module_init(t1_init);
module_exit(t1_exit);
