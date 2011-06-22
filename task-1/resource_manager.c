#include "resource_manager.h"

MODULE_LICENSE("GPL");

int t1_take_resources(struct t1_resource *resources, uint size) {
    int i;
    for (i = 0; i < size; ++i) {
        if (resources[i].init_fn() < 0) {
            t1_release_resources(resources, size);
            return -1;
        }
        resources[i].initialized = 1;
    }
    return 0;
}

// freeing taken resources in the reverse order
int t1_release_resources(struct t1_resource *resources, uint size) {
    int i;
    for (i = size - 1; i >= 0; --i) {
        if (resources[i].initialized) {
            resources[i].release_fn();
        }
    }
    return 0;
}

