#include <linux/module.h>

#include "log.h"

#define LC_ENTRY(name) {                                                             \
                        .procname       = (name),                                    \
                        .data           = NULL,                                      \
                        .maxlen         = sizeof(int),                               \
                        .mode           = S_IRUGO | S_IWUSR,                         \
                        .proc_handler   = proc_dointvec                              \
                       }

static struct ctl_table log_channels_ctls[] = { LOG_CHANNELS, {} };                    
static struct ctl_table log_channels_root[] = {
    { .procname = LOG_ROOT_NAME,
        .mode = 0555,
        .child = log_channels_ctls,
    },                                     
    {}                                     
};

#define LOG_CHANNELS_NUMBER (ARRAY_SIZE(log_channels_ctls) - 1)

static int log_levels[LOG_CHANNELS_NUMBER];
static struct ctl_table_header *log_sysctl_table_header = NULL;

inline void __log(int channel_number, int log_level, char *format, ...) {
    va_list args;
    
    if ((channel_number < LOG_CHANNELS_NUMBER) && (log_level <= log_levels[channel_number])) {
        va_start(args, format);
        vprintk(format, args);
        va_end(args);
    }
}

int register_log_channels(void) {
    int i;
    for (i = 0; i < LOG_CHANNELS_NUMBER; ++i) {
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
