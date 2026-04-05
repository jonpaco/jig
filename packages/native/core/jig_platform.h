#ifndef JIG_PLATFORM_H
#define JIG_PLATFORM_H

#include <stdarg.h>

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
    char *name;
    char *bundle_id;
    char *platform;
    char *rn_version;
    char *expo_version;
} jig_platform_app_info;

typedef struct {
    int (*screenshot)(jig_screenshot_opts *opts, jig_screenshot_result *result);
    int (*get_app_info)(jig_platform_app_info *info);
    void (*log)(const char *fmt, ...);
} jig_platform_ops;

void jig_platform_set_ops(const jig_platform_ops *ops);
const jig_platform_ops *jig_platform_get_ops(void);

#endif /* JIG_PLATFORM_H */
