#include "test_util.h"
#include "../jig_jsbridge.h"
#include "../jig_session.h"
#include "../jig_app_info.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

static char *last_bridge_sent = NULL;
static void bridge_send(const char *text, void *ud) {
    (void)ud;
    free(last_bridge_sent);
    last_bridge_sent = strdup(text);
}

static void test_lifecycle(void) {
    jig_jsbridge *bridge = jig_jsbridge_create();
    ASSERT(bridge != NULL, "lifecycle: create succeeds");
    jig_handler **handlers = jig_jsbridge_get_handlers(bridge);
    ASSERT(handlers != NULL, "lifecycle: handlers not NULL");
    ASSERT(handlers[0] != NULL, "lifecycle: handler[0] exists");
    ASSERT(handlers[1] != NULL, "lifecycle: handler[1] exists");
    ASSERT(strcmp(handlers[0]->method, "jig.internal.ready") == 0,
           "lifecycle: handler[0] is jig.internal.ready");
    ASSERT(strcmp(handlers[1]->method, "jig.internal.fiberData") == 0,
           "lifecycle: handler[1] is jig.internal.fiberData");
    jig_jsbridge_destroy(bridge);
}

static void test_no_session(void) {
    jig_jsbridge *bridge = jig_jsbridge_create();
    cJSON *result = jig_jsbridge_walk_fibers(bridge, 50, false);
    ASSERT(result == NULL, "no session: walk returns NULL");
    jig_jsbridge_destroy(bridge);
}

static void test_ready_handler(void) {
    jig_jsbridge *bridge = jig_jsbridge_create();
    jig_handler **handlers = jig_jsbridge_get_handlers(bridge);
    jig_handler *ready_handler = handlers[0];
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74", NULL);
    jig_session *sess = jig_session_create(1, app, bridge_send, NULL);
    sess->session_id = strdup("sess_internal");
    jig_error *err = NULL;
    cJSON *params = cJSON_CreateObject();
    cJSON *result = ready_handler->handle(ready_handler, params, sess, &err);
    ASSERT(err == NULL, "ready handler: no error");
    ASSERT(result != NULL, "ready handler: returns result");
    cJSON_Delete(result);
    cJSON_Delete(params);
    free(last_bridge_sent);
    last_bridge_sent = NULL;
    cJSON *walk_result = jig_jsbridge_walk_fibers(bridge, 10, false);
    ASSERT(last_bridge_sent != NULL, "ready handler: walk sent a request");
    ASSERT(walk_result == NULL, "ready handler: walk timed out (no JS response)");
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_jsbridge_destroy(bridge);
}

static cJSON *g_thread_result = NULL;

static void *walk_thread_fn(void *arg) {
    jig_jsbridge *bridge = (jig_jsbridge *)arg;
    g_thread_result = jig_jsbridge_walk_fibers(bridge, 500, false);
    return NULL;
}

static void test_walk_success(void) {
    free(last_bridge_sent);
    last_bridge_sent = NULL;
    g_thread_result = NULL;
    jig_jsbridge *bridge = jig_jsbridge_create();
    jig_handler **handlers = jig_jsbridge_get_handlers(bridge);
    jig_handler *ready_h = handlers[0];
    jig_handler *fiber_h = handlers[1];
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74", NULL);
    jig_session *sess = jig_session_create(1, app, bridge_send, NULL);
    sess->session_id = strdup("sess_internal");
    jig_error *err = NULL;
    cJSON *rparams = cJSON_CreateObject();
    cJSON *rresult = ready_h->handle(ready_h, rparams, sess, &err);
    cJSON_Delete(rresult);
    cJSON_Delete(rparams);
    pthread_t tid;
    pthread_create(&tid, NULL, walk_thread_fn, bridge);
    usleep(20000);
    ASSERT(last_bridge_sent != NULL, "walk success: request was sent to JS");
    cJSON *fparams = cJSON_CreateObject();
    cJSON *fibers = cJSON_CreateArray();
    cJSON *fiber = cJSON_CreateObject();
    cJSON_AddNumberToObject(fiber, "reactTag", 42);
    cJSON_AddStringToObject(fiber, "component", "HabitCard");
    cJSON_AddItemToArray(fibers, fiber);
    cJSON_AddItemToObject(fparams, "fibers", fibers);
    err = NULL;
    cJSON *fresult = fiber_h->handle(fiber_h, fparams, sess, &err);
    if (fresult) cJSON_Delete(fresult);
    cJSON_Delete(fparams);
    pthread_join(tid, NULL);
    ASSERT(g_thread_result != NULL, "walk success: got fiber data");
    ASSERT(cJSON_IsArray(g_thread_result), "walk success: result is array");
    ASSERT(cJSON_GetArraySize(g_thread_result) == 1, "walk success: 1 fiber entry");
    cJSON *first = cJSON_GetArrayItem(g_thread_result, 0);
    cJSON *tag = cJSON_GetObjectItem(first, "reactTag");
    ASSERT(tag && tag->valueint == 42, "walk success: correct reactTag");
    cJSON *comp = cJSON_GetObjectItem(first, "component");
    ASSERT(comp && strcmp(comp->valuestring, "HabitCard") == 0, "walk success: correct component");
    cJSON_Delete(g_thread_result);
    g_thread_result = NULL;
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_jsbridge_destroy(bridge);
}

static void test_disconnect(void) {
    jig_jsbridge *bridge = jig_jsbridge_create();
    jig_handler **handlers = jig_jsbridge_get_handlers(bridge);
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74", NULL);
    jig_session *sess = jig_session_create(1, app, bridge_send, NULL);
    sess->session_id = strdup("sess_internal");
    jig_error *err = NULL;
    cJSON *p = cJSON_CreateObject();
    cJSON *r = handlers[0]->handle(handlers[0], p, sess, &err);
    cJSON_Delete(r);
    cJSON_Delete(p);
    jig_jsbridge_on_disconnect(bridge, sess);
    free(last_bridge_sent);
    last_bridge_sent = NULL;
    cJSON *result = jig_jsbridge_walk_fibers(bridge, 10, false);
    ASSERT(result == NULL, "disconnect: walk returns NULL");
    ASSERT(last_bridge_sent == NULL, "disconnect: no message sent");
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_jsbridge_destroy(bridge);
}

static void test_walk_include_props(void) {
    free(last_bridge_sent);
    last_bridge_sent = NULL;
    jig_jsbridge *bridge = jig_jsbridge_create();
    jig_handler **handlers = jig_jsbridge_get_handlers(bridge);
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74", NULL);
    jig_session *sess = jig_session_create(1, app, bridge_send, NULL);
    sess->session_id = strdup("sess_internal");
    jig_error *err = NULL;
    cJSON *rparams = cJSON_CreateObject();
    cJSON *rresult = handlers[0]->handle(handlers[0], rparams, sess, &err);
    cJSON_Delete(rresult);
    cJSON_Delete(rparams);

    /* Walk without props */
    free(last_bridge_sent);
    last_bridge_sent = NULL;
    cJSON *result = jig_jsbridge_walk_fibers(bridge, 10, false);
    ASSERT(last_bridge_sent != NULL, "walk no props: sent request");
    ASSERT(strstr(last_bridge_sent, "includeProps") == NULL,
           "walk no props: no includeProps in message");

    /* Walk with props */
    free(last_bridge_sent);
    last_bridge_sent = NULL;
    result = jig_jsbridge_walk_fibers(bridge, 10, true);
    ASSERT(last_bridge_sent != NULL, "walk with props: sent request");
    ASSERT(strstr(last_bridge_sent, "\"includeProps\":true") != NULL,
           "walk with props: includeProps present in message");

    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_jsbridge_destroy(bridge);
}

int main(void) {
    test_lifecycle();
    test_no_session();
    test_ready_handler();
    test_walk_success();
    test_disconnect();
    test_walk_include_props();
    free(last_bridge_sent);
    last_bridge_sent = NULL;
    if (failures == 0) printf("test_jsbridge: all tests passed\n");
    else printf("test_jsbridge: %d failure(s)\n", failures);
    return failures;
}
