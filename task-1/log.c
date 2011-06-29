#include <linux/module.h>

#include "log.h"

DEFINE_LOG_CHANNELS("t1-debug");

static struct ctl_table_header *log_sysctl_table_header = NULL;

int register_log_channels(void) {
    int i;
    for (i = 0; i < log_channels_number; ++i) {
        log_channels_ctls[i].data = &log_levels[i];
    }
    log_sysctl_table_header = register_sysctl_table(&log_channels_root[0]);

    if (log_sysctl_table_header == NULL) {
        ERR("t1: cannot register sysctl log table\n");
        return -1;
    }

    return 0;
}

void unregister_log_channels(void) {
    unregister_sysctl_table(log_sysctl_table_header);
}
