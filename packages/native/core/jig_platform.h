#ifndef JIG_PLATFORM_H
#define JIG_PLATFORM_H

#include <stdarg.h>
#include "jig_app_info.h"

typedef struct {
    unsigned char *data;   /* JPEG/PNG bytes, caller frees */
    int length;
    int width;
    int height;
    const char *format;    /* "jpeg" or "png" */
} jig_screenshot_result;

typedef struct {
    int quality;           /* 0-100 for JPEG */
    const char *format;    /* "jpeg" or "png" */
} jig_screenshot_opts;

typedef struct {
    int (*screenshot)(jig_screenshot_opts *opts, jig_screenshot_result *result);
    int (*get_app_info)(jig_app_info *info);
    void (*log)(const char *fmt, ...);
} jig_platform_ops;

void jig_platform_set_ops(const jig_platform_ops *ops);
const jig_platform_ops *jig_platform_get_ops(void);

#endif /* JIG_PLATFORM_H */
