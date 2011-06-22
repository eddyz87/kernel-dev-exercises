#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

#include <linux/init.h>
#include <linux/module.h>

// TODO: will have to play a bit with preprocessor to split it between init/exit sections
struct t1_resource {
    uint initialized:1;
    int (*init_fn) (void);      // less then 0 on error
    void (*release_fn) (void);
};

#define RC_INIT(init_fn, release_fn) { 0, (init_fn), (release_fn) }

int t1_take_resources(struct t1_resource *resources, uint size);
int t1_release_resources(struct t1_resource *resources, uint size);

#endif /*__RESOURCE_MANAGER_H__*/
