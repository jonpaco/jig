#include <string.h>
#include <stdlib.h>
#include "../jig_app_info.h"
#include "test_util.h"

static void test_create_with_all_fields(void) {
    jig_app_info *info = jig_app_info_create(
        "HabitTracker", "com.habit.tracker", "ios", "0.76.0", "52.0.0");

    ASSERT(info != NULL, "info should not be NULL");
    ASSERT(strcmp(info->name, "HabitTracker") == 0, "name");
    ASSERT(strcmp(info->bundle_id, "com.habit.tracker") == 0, "bundle_id");
    ASSERT(strcmp(info->platform, "ios") == 0, "platform");
    ASSERT(strcmp(info->rn_version, "0.76.0") == 0, "rn_version");
    ASSERT(strcmp(info->expo_version, "52.0.0") == 0, "expo_version");

    jig_app_info_free(info);
}

static void test_create_without_expo(void) {
    jig_app_info *info = jig_app_info_create(
        "BareApp", "com.bare.app", "android", "0.75.0", NULL);

    ASSERT(info != NULL, "info should not be NULL");
    ASSERT(info->expo_version == NULL, "expo_version should be NULL");

    jig_app_info_free(info);
}

static void test_to_json_all_fields(void) {
    jig_app_info *info = jig_app_info_create(
        "HabitTracker", "com.habit.tracker", "ios", "0.76.0", "52.0.0");

    cJSON *json = jig_app_info_to_json(info);
    ASSERT(json != NULL, "json should not be NULL");

    cJSON *name = cJSON_GetObjectItem(json, "name");
    ASSERT(name != NULL && cJSON_IsString(name), "json name is string");
    ASSERT(strcmp(name->valuestring, "HabitTracker") == 0, "json name value");

    cJSON *bid = cJSON_GetObjectItem(json, "bundleId");
    ASSERT(bid != NULL && cJSON_IsString(bid), "json bundleId is string");
    ASSERT(strcmp(bid->valuestring, "com.habit.tracker") == 0, "json bundleId value");

    cJSON *plat = cJSON_GetObjectItem(json, "platform");
    ASSERT(plat != NULL && cJSON_IsString(plat), "json platform is string");
    ASSERT(strcmp(plat->valuestring, "ios") == 0, "json platform value");

    cJSON *rn = cJSON_GetObjectItem(json, "rn");
    ASSERT(rn != NULL && cJSON_IsString(rn), "json rn is string");
    ASSERT(strcmp(rn->valuestring, "0.76.0") == 0, "json rn value");

    cJSON *expo = cJSON_GetObjectItem(json, "expo");
    ASSERT(expo != NULL && cJSON_IsString(expo), "json expo is string");
    ASSERT(strcmp(expo->valuestring, "52.0.0") == 0, "json expo value");

    cJSON_Delete(json);
    jig_app_info_free(info);
}

static void test_to_json_no_expo(void) {
    jig_app_info *info = jig_app_info_create(
        "BareApp", "com.bare.app", "android", "0.75.0", NULL);

    cJSON *json = jig_app_info_to_json(info);
    ASSERT(json != NULL, "json should not be NULL");

    cJSON *expo = cJSON_GetObjectItem(json, "expo");
    ASSERT(expo == NULL, "json should not have expo key when NULL");

    /* other fields should still be present */
    cJSON *name = cJSON_GetObjectItem(json, "name");
    ASSERT(name != NULL, "json name should be present");

    cJSON_Delete(json);
    jig_app_info_free(info);
}

int main(void) {
    test_create_with_all_fields();
    test_create_without_expo();
    test_to_json_all_fields();
    test_to_json_no_expo();

    if (failures > 0) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All app_info tests passed.\n");
    return 0;
}
