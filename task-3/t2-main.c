#include <linux/init.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/list.h>

#include <linux/slab.h>
//#include <linux/workqueue.h>
//#include <linux/kthread.h>
//#include <linux/completion.h>
//#include <linux/percpu.h>

// need this for mouse interrupt number: I8042_AUX_IRQ -- PS/2 mouse
//#include <drivers/input/serio/i8042-io.h>
#define I8042_AUX_IRQ 12

/*
 TODO:
    - spin locks
    - vmalloc/kmalloc
    - sem up/down
 */

#include "log.h"

MODULE_LICENSE("GPL");

struct context {
    struct list_head list;
    int irq_registered;
};

static LIST_HEAD(contexts);

#define DEF_CONTEXT(name, arg) struct context* name = (struct context*)(arg)

irqreturn_t mouse_irq(int irq, void *arg) {
    DEF_CONTEXT(ctx, arg);
    LOGRL("got mouse interrupt: %i:%p", irq, ctx);
    return IRQ_NONE;
}

static void init_context(struct context *ctx) {
    INIT_LIST_HEAD(&ctx->list);
    ctx->irq_registered = false;
}

static void free_context(struct context *ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->irq_registered) {
        free_irq(I8042_AUX_IRQ, ctx);
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
