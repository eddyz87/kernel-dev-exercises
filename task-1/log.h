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

// the name of the debug channels entry in the /proc/sys
#define LOG_ROOT_NAME "t1-debug"

#define LOG(channel_number, log_level, format, ...)                                                             \
    __log((channel_number), (log_level), KERN_DEBUG "%s:%d:%s: " format, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define ERR(format, ...)                                                                                        \
    printk(KERN_ERR "%s:%d:%s: " format, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

inline void __log(int channel_number, int log_level, char *format, ...);
int register_log_channels(void);
void unregister_log_channels(void);

#endif /* __LOG_H__ */
