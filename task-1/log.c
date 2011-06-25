#include <linux/module.h>

// well, this could be a bit to much for the purpose,
// but it's a playground anyway...

static u32 channels_status; // 32 channels, all disabled by default
static u8  log_levels[32];  // log level per each channel

#define FOO 0
#define BAR 1

#define CH_LOG_LEVEL(n) (log_levels[(n) & 0x1F])

#define LOG(channel_number, log_level, format, ...)                        \
            if (((1 << (channel_number)) & channels_status) &&             \
                ((log_level) <= CH_LOG_LEVEL(channel_number))) {           \
                    printk(KERN_DEBUG "%s:%d:%s: " format,                 \
                        __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);  \
            } 

static void set_log_level(u32 channel_number, u8 log_level) {
    CH_LOG_LEVEL(channel_number) = log_level;
}

static void test(void) {
    LOG(FOO, 1, "hello %s\n", "world");
}
