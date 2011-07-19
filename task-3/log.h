#ifndef __LOG_H__
#define __LOG_H__

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/ratelimit.h>

#define LOG(format, ...) \
    printk(KERN_DEBUG "%s:%d " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define ERR(format, ...) \
    printk(KERN_ERR "%s:%d:%s " format "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)

#define LOG_RL(format, ...) \
    printk_ratelimited(KERN_DEBUG "%s:%d " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define ERR_RL(format, ...) \
    printk_ratelimited(KERN_ERR "%s:%d:%s " format "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#else

#define LOG_RL(format, ...)          \
    do {                            \
        if (printk_ratelimit()) {   \
            LOG(format, ##__VA_ARGS__);     \
        }                           \
    } while (0)

#define ERR_RL(format, ...)          \
    do {                            \
        if (printk_ratelimit()) {   \
            ERR(format, ##__VA_ARGS__);     \
        }                           \
    } while (0)

#endif

#endif /* __LOG_H__ */
