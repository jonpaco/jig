#include "test_util.h"
#include "../handlers/jig_handshake.h"
#include <stdlib.h>
#include <string.h>

static char *last_sent = NULL;
static void capture_send(const char *text, void *ud) {
    (void)ud;
    free(last_sent);
    last_sent = strdup(text);
}

static void clear_sent(void) {
    free(last_sent);
    last_sent = NULL;
}

static void test_valid_handshake(void) {
    clear_sent();
    jig_handler *hs = jig_handshake_create();
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74.0", NULL);
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    cJSON *params = cJSON_CreateObject();
    cJSON *proto = cJSON_CreateObject();
    cJSON_AddNumberToObject(proto, "version", 1);
    cJSON_AddItemToObject(params, "protocol", proto);

    jig_error *err = NULL;
    cJSON *result = hs->handle(hs, params, sess, &err);

    ASSERT(err == NULL, "valid handshake: no error");
    ASSERT(result != NULL, "valid handshake: has result");

    cJSON *sid = cJSON_GetObjectItem(result, "sessionId");
    ASSERT(sid != NULL && cJSON_IsString(sid), "valid handshake: has sessionId");
    ASSERT(strncmp(sid->valuestring, "sess_", 5) == 0,
           "valid handshake: sessionId starts with sess_");
    ASSERT(strlen(sid->valuestring) == 13,
           "valid handshake: sessionId is sess_ + 8 hex chars");

    cJSON *neg = cJSON_GetObjectItem(result, "negotiated");
    ASSERT(neg != NULL, "valid handshake: has negotiated");
    cJSON *negp = cJSON_GetObjectItem(neg, "protocol");
    ASSERT(negp != NULL && negp->valueint == 1,
           "valid handshake: negotiated protocol is 1");

    cJSON *enabled = cJSON_GetObjectItem(result, "enabled");
    ASSERT(enabled != NULL && cJSON_IsArray(enabled) && cJSON_GetArraySize(enabled) == 0,
           "valid handshake: enabled is empty array");

    cJSON *rejected = cJSON_GetObjectItem(result, "rejected");
    ASSERT(rejected != NULL && cJSON_IsArray(rejected) && cJSON_GetArraySize(rejected) == 0,
           "valid handshake: rejected is empty array");

    cJSON_Delete(result);
    cJSON_Delete(params);
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_handshake_destroy(hs);
}

static void test_missing_params(void) {
    clear_sent();
    jig_handler *hs = jig_handshake_create();
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74.0", NULL);
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    jig_error *err = NULL;
    cJSON *result = hs->handle(hs, NULL, sess, &err);

    ASSERT(result == NULL, "missing params: no result");
    ASSERT(err != NULL, "missing params: has error");
    ASSERT(err->code == JIG_ERROR_INVALID_PARAMS,
           "missing params: invalid params error code");

    jig_error_free(err);
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_handshake_destroy(hs);
}

static void test_wrong_version(void) {
    clear_sent();
    jig_handler *hs = jig_handshake_create();
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74.0", NULL);
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    cJSON *params = cJSON_CreateObject();
    cJSON *proto = cJSON_CreateObject();
    cJSON_AddNumberToObject(proto, "version", 99);
    cJSON_AddItemToObject(params, "protocol", proto);

    jig_error *err = NULL;
    cJSON *result = hs->handle(hs, params, sess, &err);

    ASSERT(result == NULL, "wrong version: no result");
    ASSERT(err != NULL, "wrong version: has error");
    ASSERT(err->code == JIG_ERROR_PROTOCOL_VERSION,
           "wrong version: protocol version error code");

    jig_error_free(err);
    cJSON_Delete(params);
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_handshake_destroy(hs);
}

int main(void) {
    test_valid_handshake();
    test_missing_params();
    test_wrong_version();

    clear_sent();

    if (failures == 0) {
        printf("test_handshake: all tests passed\n");
    } else {
        printf("test_handshake: %d failure(s)\n", failures);
    }
    return failures;
}
