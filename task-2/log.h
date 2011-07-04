#ifndef __LOG_H__
#define __LOG_H__

#include <linux/kernel.h>

#define LOG(format, ...) \
    printk(KERN_DEBUG "%s:%d " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define ERR(format, ...) \
    printk(KERN_ERR "%s:%d:%s " format "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#endif /* __LOG_H__ */
