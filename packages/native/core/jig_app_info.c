#include "jig_app_info.h"
#include <stdlib.h>
#include <string.h>

static char *safe_strdup(const char *s) {
    return s ? strdup(s) : NULL;
}

jig_app_info *jig_app_info_create(const char *name,
                                   const char *bundle_id,
                                   const char *platform,
                                   const char *rn_version,
                                   const char *expo_version) {
    if (!name || !bundle_id || !platform || !rn_version) return NULL;

    jig_app_info *info = calloc(1, sizeof(jig_app_info));
    if (!info) return NULL;

    info->name = strdup(name);
    info->bundle_id = strdup(bundle_id);
    info->platform = strdup(platform);
    info->rn_version = strdup(rn_version);
    info->expo_version = safe_strdup(expo_version);

    if (!info->name || !info->bundle_id || !info->platform || !info->rn_version) {
        jig_app_info_free(info);
        return NULL;
    }

    return info;
}

void jig_app_info_free(jig_app_info *info) {
    if (!info) return;
    free(info->name);
    free(info->bundle_id);
    free(info->platform);
    free(info->rn_version);
    free(info->expo_version);
    free(info);
}

cJSON *jig_app_info_to_json(const jig_app_info *info) {
    if (!info) return NULL;

    cJSON *obj = cJSON_CreateObject();
    if (!obj) return NULL;

    cJSON_AddStringToObject(obj, "name", info->name);
    cJSON_AddStringToObject(obj, "bundleId", info->bundle_id);
    cJSON_AddStringToObject(obj, "platform", info->platform);
    cJSON_AddStringToObject(obj, "rn", info->rn_version);

    if (info->expo_version) {
        cJSON_AddStringToObject(obj, "expo", info->expo_version);
    }

    return obj;
}
