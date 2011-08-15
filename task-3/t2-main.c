#include <linux/init.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
//#include <linux/percpu.h>

#include "log.h"

// need this for mouse interrupt number: I8042_AUX_IRQ -- PS/2 mouse
//#include <drivers/input/serio/i8042-io.h>
#define I8042_AUX_IRQ 12


MODULE_LICENSE("GPL");

#define TIMER_PERIOD_MSEC    10
#define TIMER_DELAY_MSEC     10
#define THREAD_PERIOD_MSEC   10

#define mod_timer_ms(timer, msec) \
    mod_timer((timer), msecs_to_jiffies(msec))

#define RC_SIZE 10

#define NULLIFY(ptr) memset((ptr), 0, sizeof(typeof(*(ptr))))

typedef int common_resource[RC_SIZE];

struct context {
    struct list_head list;
    
    int irq_registered;
    
    atomic_t cycles_count;
    atomic_t failures_count;
    atomic_t mouse_irqs_count;
    atomic_t thread_syncs_count;
    atomic_t tasklet_syncs_count;

    struct timer_list single_shot_timer;
    struct timer_list periodic_timer;

    common_resource rc_irq;
    common_resource rc_tasklet;
    common_resource rc_irq_tasklet;
    common_resource rc_thread;
    common_resource rc_tasklet_thread;

    spinlock_t irq_lock;
    spinlock_t tasklet_lock;
    spinlock_t irq_tasklet_lock;
    spinlock_t tasklet_thread_lock;
    struct mutex thread_lock;
    
    struct tasklet_struct tasklet_a;
    struct tasklet_struct tasklet_b;

    struct task_struct *thread_a;
    struct task_struct *thread_b;
};

static LIST_HEAD(contexts);

unsigned int get_random_int(void) {
    unsigned int v;
    get_random_bytes(&v, sizeof(v));
    return v;
}

static void write_rc(struct context *ctx, common_resource rc, const char *name) {
    int i;
    int v;
    int sum1 = 0;
    int sum2 = 0;
    int n = RC_SIZE;

    for (i = 0; i < n; ++i) {
        v = get_random_int();
        rc[i] = v;
        sum1 += v;
    }

    for (i = 0; i < n; ++i) {
        sum2 += rc[i];
    }

    atomic_inc(&ctx->cycles_count);

    if (sum1 != sum2) {
        atomic_inc(&ctx->failures_count);
        ERR_RL("sync failure in: %s", name);
    }

    LOG_RL("count is: %i/%i/%i/%i/%i", atomic_read(&ctx->cycles_count),
                                       atomic_read(&ctx->failures_count),
                                       atomic_read(&ctx->mouse_irqs_count),
                                       atomic_read(&ctx->thread_syncs_count),
                                       atomic_read(&ctx->tasklet_syncs_count));
}

static void write_rc_irq(struct context *ctx, const char *name) {
    unsigned long flags;
    
    spin_lock_irqsave(&ctx->irq_lock, flags);
    write_rc(ctx, ctx->rc_irq, name); 
    spin_unlock_irqrestore(&ctx->irq_lock, flags);
    
    spin_lock_irqsave(&ctx->irq_tasklet_lock, flags);
    write_rc(ctx, ctx->rc_irq_tasklet, name); 
    spin_unlock_irqrestore(&ctx->irq_tasklet_lock, flags);
}

#define DEF_CONTEXT(name, arg) \
    struct context* name = (struct context*)(arg)

static void timer_func(unsigned long arg) {
    DEF_CONTEXT(ctx, arg);
    write_rc_irq(ctx, __FUNCTION__);
}

static void periodic_timer_func(unsigned long arg) {
    DEF_CONTEXT(ctx, arg);
    write_rc_irq(ctx, __FUNCTION__);
    mod_timer_ms(&ctx->periodic_timer, TIMER_PERIOD_MSEC);
}

static irqreturn_t mouse_irq(int irq, void *arg) {
    DEF_CONTEXT(ctx, arg);

    atomic_inc(&ctx->mouse_irqs_count);
    //LOG_RL("got mouse interrupt: %i:%p", irq, ctx);
    
    switch (get_random_int() % 4) {
    case 0:
        mod_timer_ms(&ctx->single_shot_timer, TIMER_DELAY_MSEC);
        break;
    case 1:
        tasklet_schedule(&ctx->tasklet_a);
        break;
    case 2:
        tasklet_schedule(&ctx->tasklet_b);
        break;
    default:
        write_rc_irq(ctx, __FUNCTION__);
    }

    return IRQ_NONE;
}

static void write_rc_tasklet(struct context *ctx, const char *name) {
    unsigned long flags;
    
    spin_lock(&ctx->tasklet_lock);
    write_rc(ctx, ctx->rc_tasklet, name); 
    spin_unlock(&ctx->tasklet_lock);
    
    spin_lock_irqsave(&ctx->irq_tasklet_lock, flags);
    write_rc(ctx, ctx->rc_irq_tasklet, name); 
    spin_unlock_irqrestore(&ctx->irq_tasklet_lock, flags);
    
    spin_lock(&ctx->tasklet_thread_lock);
    write_rc(ctx, ctx->rc_tasklet_thread, name); 
    spin_unlock(&ctx->tasklet_thread_lock);

    atomic_inc(&ctx->tasklet_syncs_count);
}

static void tasklet_func(unsigned long arg) {
    DEF_CONTEXT(ctx, arg);
    write_rc_tasklet(ctx, __FUNCTION__);
}

static void write_rc_thread(struct context *ctx, const char *name) {
    atomic_inc(&ctx->thread_syncs_count);

    mutex_lock(&ctx->thread_lock);
    write_rc(ctx, ctx->rc_thread, name); 
    mutex_unlock(&ctx->thread_lock);
    
    spin_lock_bh(&ctx->tasklet_thread_lock);
    write_rc(ctx, ctx->rc_tasklet_thread, name); 
    spin_unlock_bh(&ctx->tasklet_thread_lock);
}

static int thread_func(void *arg) {
    DEF_CONTEXT(ctx, arg);

    while (!kthread_should_stop()) {
        write_rc_thread(ctx, __FUNCTION__);
        msleep_interruptible(THREAD_PERIOD_MSEC); // could be awaken by stop call
    }

    return 0;
}

#define callback(ctx, action, field, func) \
    action(&((ctx)->field), (func), (unsigned long) (ctx))

static void init_context(struct context *ctx) {
    NULLIFY(ctx);
    
    INIT_LIST_HEAD(&ctx->list);
    ctx->irq_registered = false;
    atomic_set(&ctx->failures_count, 0);
    atomic_set(&ctx->cycles_count, 0);
    atomic_set(&ctx->mouse_irqs_count, 0);
    atomic_set(&ctx->thread_syncs_count, 0);
    
    callback(ctx, setup_timer, periodic_timer, periodic_timer_func);
    callback(ctx, setup_timer, single_shot_timer, timer_func);

    spin_lock_init(&ctx->irq_lock);
    spin_lock_init(&ctx->tasklet_lock);
    spin_lock_init(&ctx->irq_tasklet_lock);
    spin_lock_init(&ctx->tasklet_thread_lock);
    mutex_init(&ctx->thread_lock);
    
    callback(ctx, tasklet_init, tasklet_a, tasklet_func);
    callback(ctx, tasklet_init, tasklet_b, tasklet_func);
}

#undef callback

static void free_context(struct context *ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->irq_registered) {
        free_irq(I8042_AUX_IRQ, ctx);
    }

    tasklet_kill(&ctx->tasklet_a);
    tasklet_kill(&ctx->tasklet_b);

    del_timer_sync(&ctx->periodic_timer);
    del_timer_sync(&ctx->single_shot_timer);

    if (ctx->thread_a) {
        kthread_stop(ctx->thread_a);
    }
    if (ctx->thread_b) {
        kthread_stop(ctx->thread_b);
    }

    kfree(ctx);
}

static struct context* new_context(void) {
    struct context* ctx = kmalloc(sizeof(struct context), GFP_KERNEL);
    if (!ctx) {
        ERR("cannot allocate context");
        goto err;
    }

    init_context(ctx);
    
    if (request_irq(I8042_AUX_IRQ, mouse_irq, IRQF_SHARED, "t2", ctx)) {
        ERR("cannot request irq");
        goto err;
    }
    ctx->irq_registered = true;
    
    mod_timer_ms(&ctx->periodic_timer, TIMER_PERIOD_MSEC);

    ctx->thread_a = kthread_create(thread_func, ctx, "r2d2");
    if (!ctx->thread_a) {
        ERR("cannot allocate thread a");
        goto err;
    }

    ctx->thread_b = kthread_create(thread_func, ctx, "c3p0");
    if (!ctx->thread_b) {
        ERR("cannot allocate thread a");
        goto err;
    }
    
    wake_up_process(ctx->thread_a);
    wake_up_process(ctx->thread_b);

    list_add(&ctx->list, &contexts);
    return ctx;

err:
    free_context(ctx);
    return NULL;
}

static int __init start(void) {
    LOG("t2 init started");

    if (!new_context()) {
        ERR("t2 could not initialize");
        return -1;
    }

    LOG("t2 init ok");
    return 0;
}

static void __exit stop(void) {
    struct context *ctx, *next;
    LOG("enter");

    list_for_each_entry_safe(ctx, next, &contexts, list) {
        list_del(&ctx->list);
        free_context(ctx);
    }

    LOG("leave");
}

module_init(start);
module_exit(stop);
