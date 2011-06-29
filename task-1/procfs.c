#include <linux/module.h>
#include <linux/sysctl.h>

#include "log.h"
#include "params.h"

MODULE_LICENSE("GPL");

// TODO: functions are almost the same, should abstract them to wrapper with proc_handler and data printer parameters
static int t1_proc_dostring(struct ctl_table *table, int write, void __user *buffer, size_t *lenp, loff_t *ppos) {
    int result;

    LOG(LC_PROC, 0, "t1: gona read %s from procfs entry, old value is: %s\n", table->procname, (char*)table->data);
    result = proc_dostring(table, write, buffer, lenp, ppos);
    LOG(LC_PROC, 0, "t1: read %s from procfs, new value is: %s\n", table->procname, (char*)table->data);
    
    return result;
}

static int t1_proc_doint(struct ctl_table *table, int write, void __user *buffer, size_t *lenp, loff_t *ppos) {
    int result;

    LOG(LC_PROC, 0, "t1: gona read %s from procfs entry, old value is: %i\n", table->procname, *((int*)table->data));
    result = proc_dointvec(table, write, buffer, lenp, ppos);
    LOG(LC_PROC, 0, "t1: read %s from procfs, new value is: %i\n", table->procname, *((int*)table->data));

    return result;
}

static struct ctl_table t1_sysctl_table[] = {
    {
        .procname       = "do-stupid-things",
        .data           = &t1_do_stupid_things,
        .maxlen         = sizeof(int),
        .mode           = S_IRUGO | S_IWUSR,
        .proc_handler   = t1_proc_doint
    },
    {
        .procname       = "stupid-thing-kind",
        .data           = t1_stupid_thing_kind,
        .maxlen         = sizeof(t1_stupid_thing_kind),
        .mode           = S_IRUGO | S_IWUSR,
        .proc_handler   = t1_proc_dostring
    },
    {}
};

struct ctl_table_header *t1_sysctl_table_header = NULL;

int t1_procfs_register(void) {
    t1_sysctl_table_header = register_sysctl_table(&t1_sysctl_table[0]);

    if (t1_sysctl_table_header == NULL) {
        ERR(KERN_ALERT "t1: cannot register sysctl table\n");
        return -1;
    }

    LOG(LC_PROC, 0, "t1: registered sysctl table\n");
    return 0;
}

void t1_procfs_unregister(void) {
    // resource manager wouldn't call us without proper initialization
    unregister_sysctl_table(t1_sysctl_table_header);
    LOG(LC_PROC, 0, "t1: unregistered sysctl table\n");
}

