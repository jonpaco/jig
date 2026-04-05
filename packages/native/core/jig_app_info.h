#ifndef JIG_APP_INFO_H
#define JIG_APP_INFO_H

#include "vendor/cJSON/cJSON.h"

typedef struct {
    char *name;          /* heap-allocated */
    char *bundle_id;     /* heap-allocated */
    char *platform;      /* heap-allocated, e.g. "ios" or "android" */
    char *rn_version;    /* heap-allocated */
    char *expo_version;  /* heap-allocated, NULL if not an Expo app */
} jig_app_info;

jig_app_info *jig_app_info_create(const char *name,
                                   const char *bundle_id,
                                   const char *platform,
                                   const char *rn_version,
                                   const char *expo_version);

void jig_app_info_free(jig_app_info *info);

/* Returns a new cJSON object. Caller owns it (free with cJSON_Delete). */
cJSON *jig_app_info_to_json(const jig_app_info *info);

#endif /* JIG_APP_INFO_H */
