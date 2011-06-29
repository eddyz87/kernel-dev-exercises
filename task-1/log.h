#ifndef __LOG_H__
#define __LOG_H__

// well, this could be a bit to much for the purpose,
// but it's a playground anyway...

#include <linux/sysctl.h>

//TODO: define log levels with meaningfull names?

// channel number constants
#define LC_MAIN 0
#define LC_PROC 1
#define LC_THIS 2
#define LC_THAT 3

// channel number names
#define LOG_CHANNELS LC_ENTRY("main"),       \
                     LC_ENTRY("proc"),       \
                     LC_ENTRY("this"),       \
                     LC_ENTRY("that")

// internal stuff
extern struct ctl_table log_channels_root[];
extern struct ctl_table log_channels_ctls[];
extern int log_levels[];
extern int log_channels_number;

#define LC_ENTRY(name) {                                                             \
                        .procname       = (name),                                    \
                        .data           = NULL,                                      \
                        .maxlen         = sizeof(int),                               \
                        .mode           = S_IRUGO | S_IWUSR,                         \
                        .proc_handler   = proc_dointvec                              \
                       }

// have not figured a way to define channel number constants along with this....
#define DEFINE_LOG_CHANNELS(name)                                                   \
    struct ctl_table log_channels_ctls[] = { LOG_CHANNELS, {} };                    \
    struct ctl_table log_channels_root[] = { { .procname = (name),                  \
                                               .mode = 0555,                        \
                                               .child = log_channels_ctls,          \
                                             },                                     \
                                             {}                                     \
                                           };                                       \
    int log_channels_number = ARRAY_SIZE(log_channels_ctls) - 1;                    \
    int log_levels[ARRAY_SIZE(log_channels_ctls) - 1]

#define LOG(channel_number, log_level, format, ...)                                 \
            if ((channel_number < log_channels_number) &&                           \
                ((log_level) <= log_levels[channel_number])) {                      \
                    printk(KERN_DEBUG "%s:%d:%s: " format,                          \
                        __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);           \
            } 

#define ERR(format, ...)                                                            \
            printk(KERN_ERR "%s:%d:%s: " format,                                    \
                        __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

int register_log_channels(void);
void unregister_log_channels(void);

#endif /* __LOG_H__ */
