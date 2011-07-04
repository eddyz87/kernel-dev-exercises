#include <linux/init.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>

#include "log.h"

MODULE_LICENSE("GPL");

#define TIMER_A_PERIOD_MSEC     10
#define TIMER_B_PERIOD_MSEC     10
#define TIMER_C_PERIOD_MSEC     10
#define THREAD_PERIOD_MSEC      100

#define TIMER_B_FIRES_LIMIT     10
#define TIMER_C_FIRES_LIMIT     10
#define THREAD_FIRES_LIMIT      10

#define WORKER_CYCLES           5

#define mod_timer_ms(timer, msec) mod_timer((timer), msecs_to_jiffies(msec))

// Q, investigate: different final value of the counter from launch to launch
//                 is it lack of synchronization or i've made a mistake ?

// TODO: make some stats in procfs,
//       trigger new wave of counting by write to procfs entry

struct context;

// TODO: find out wether I could use 'data' field from 'work_struct'
struct my_work {
    struct work_struct work;
    struct context *context;
};

struct context {
    int shared_counter;

    struct timer_list timer_a;     // fires once
    struct timer_list timer_b;     // reactivates itself at each fire
    struct timer_list timer_c;     // fires once and launches tasklet and workqueue task

    struct tasklet_struct timer_c_tasklet;
    struct my_work work;
    struct workqueue_struct *work_queue;
    struct task_struct *thread;

    int timer_b_fires;
    int timer_c_fires;
};

static void inc_shared_counter(struct context* ctx, char *prefix) {
    int old = ctx->shared_counter;
    int new = ++(ctx->shared_counter);

    LOG("%s: counter shift: %d -> %d%s", prefix, old, new, old + 1 == new ? "" : " RACE detected");
}

#define DEF_CONTEXT(name, arg) struct context* name = (struct context*)(arg)

static void timer_a_func(unsigned long arg) {
    DEF_CONTEXT(ctx, arg);
    inc_shared_counter(ctx, "timer A");
}

static void timer_b_func(unsigned long arg) {
    DEF_CONTEXT(ctx, arg);
    
    ctx->timer_b_fires += 1;
    inc_shared_counter(ctx, "timer B");

    if (ctx->timer_b_fires < TIMER_B_FIRES_LIMIT) {
        mod_timer_ms(&(ctx->timer_b), TIMER_B_PERIOD_MSEC);
    }
}

static void timer_c_func(unsigned long arg) {
    DEF_CONTEXT(ctx, arg);
    
    ctx->timer_c_fires += 1;
    inc_shared_counter(ctx, "timer C");
    
    tasklet_schedule(&(ctx->timer_c_tasklet));
    queue_work(ctx->work_queue, &(ctx->work.work));
}

static void timer_c_tasklet_func(unsigned long arg) {
    DEF_CONTEXT(ctx, arg);

    inc_shared_counter(ctx, "timer C tasklet");

    if (ctx->timer_c_fires < TIMER_C_FIRES_LIMIT) {
        mod_timer_ms(&(ctx->timer_c), TIMER_C_PERIOD_MSEC);
    }
}

struct context global_contex;

static void worker_func(struct my_work *work) {
    DEF_CONTEXT(ctx, work->context);
    int i;

    //LOG("ctx == ~%p, global ctx is %p", ctx, &global_contex);
    for (i = 0; i < WORKER_CYCLES; ++i) {
        inc_shared_counter(ctx, "worker");
    }
}

static int thread_func(void *arg) {
    DEF_CONTEXT(ctx, arg);
    int count = 0;

    while (!kthread_should_stop()) {
        if (count < THREAD_FIRES_LIMIT) {
            inc_shared_counter(ctx, "thread");
            ++count;
        }
        msleep_interruptible(THREAD_PERIOD_MSEC); // could be awaken by stop call
    }

    return 0;
}

#define callback(ctx, action, field, func) action(&((ctx)->field), (func), (unsigned long) (ctx))

static int init_context(struct context* ctx) {
    ctx->shared_counter = 0;
    ctx->timer_b_fires = 0;
    ctx->timer_c_fires = 0;

    callback(ctx, setup_timer, timer_a, timer_a_func);
    callback(ctx, setup_timer, timer_b, timer_b_func);
    callback(ctx, setup_timer, timer_c, timer_c_func);
    callback(ctx, tasklet_init, timer_c_tasklet, timer_c_tasklet_func);

    INIT_WORK(&(ctx->work.work), (work_func_t)worker_func);
    ctx->work.context = ctx;
    
    ctx->work_queue = create_workqueue("r2d2");
    if (!ctx->work_queue) {
        ERR("cannot allocate work queue");
        return -1;
    }

    ctx->thread = kthread_create(thread_func, ctx, "c3po");
    if (!ctx->thread) {
        ERR("cannot allocate thread");
        return -1;
    }
    
    return 0;
}

#undef callback

static void start_context(struct context* ctx) {
    mod_timer_ms(&(ctx->timer_a), TIMER_A_PERIOD_MSEC);
    mod_timer_ms(&(ctx->timer_b), TIMER_B_PERIOD_MSEC);
    mod_timer_ms(&(ctx->timer_c), TIMER_C_PERIOD_MSEC);
    wake_up_process(ctx->thread);
}

static void stop_context(struct context* ctx) {
    del_timer_sync(&(ctx->timer_a));
    del_timer_sync(&(ctx->timer_b));
    del_timer_sync(&(ctx->timer_c));
    tasklet_disable(&(ctx->timer_c_tasklet));
    if (ctx->work_queue) {
        flush_workqueue(ctx->work_queue);
        destroy_workqueue(ctx->work_queue);
    }
    if (ctx->thread) {
        kthread_stop(ctx->thread);
    }
}

static int __init start(void) {
    LOG("enter");
    if (init_context(&global_contex) < 0) {
        ERR("t2 could not initialize");
        stop_context(&global_contex);
    } else {
        start_context(&global_contex);
    }
    LOG("leave");
    return 0;
}

static void __exit stop(void) {
    LOG("enter");
    stop_context(&global_contex);
    LOG("leave");
}

module_init(start);
module_exit(stop);
