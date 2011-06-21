#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

static int __init t1_init(void) {
    printk(KERN_ALERT "Module t1 is being loaded\n");
    return 0;
}

static void __exit t1_exit(void) {
    printk(KERN_ALERT "Module t1 is being unloaded\n");
}

module_init(t1_init);
module_exit(t1_exit);
