#include "jig_platform.h"
#include <stddef.h>

static const jig_platform_ops *g_ops = NULL;

void jig_platform_set_ops(const jig_platform_ops *ops) {
    g_ops = ops;
}

const jig_platform_ops *jig_platform_get_ops(void) {
    return g_ops;
}
