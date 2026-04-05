#include "test_util.h"
#include "../jig_dispatcher.h"
#include "../handlers/jig_handshake.h"
#include "../jig_platform.h"
#include <stdlib.h>
#include <string.h>

/* --- capture helper --- */
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

/* --- stub handler --- */
static cJSON *echo_handle(jig_handler *self, cJSON *params,
                           jig_session *session, jig_error **err) {
    (void)self; (void)session; (void)err;
    return cJSON_Duplicate(params, 1);
}

static jig_handler echo_handler = {
    .method = "test.echo",
    .thread_target = JIG_THREAD_WEBSOCKET,
    .handle = echo_handle,
    .user_data = NULL,
};

/* --- main thread mock helpers --- */
typedef struct {
    void (*callback)(void *ctx);
    void *ctx;
} captured_main_thread_call;

static captured_main_thread_call g_captured = {NULL, NULL};

static void mock_run_on_main_thread(void (*callback)(void *ctx), void *ctx) {
    g_captured.callback = callback;
    g_captured.ctx = ctx;
}

static void flush_main_thread(void) {
    if (g_captured.callback) {
        g_captured.callback(g_captured.ctx);
        g_captured.callback = NULL;
        g_captured.ctx = NULL;
    }
}

/* --- main thread handler stubs --- */
static cJSON *main_thread_handle(jig_handler *self, cJSON *params,
                                  jig_session *session, jig_error **err) {
    (void)self; (void)session; (void)err;
    cJSON *result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "dispatched", "on_main");
    return result;
}

static jig_handler main_thread_handler = {
    .method = "test.mainthread",
    .thread_target = JIG_THREAD_MAIN,
    .handle = main_thread_handle,
    .user_data = NULL,
};

static cJSON *main_thread_error_handle(jig_handler *self, cJSON *params,
                                        jig_session *session, jig_error **err) {
    (void)self; (void)params; (void)session;
    *err = jig_error_internal("main thread error");
    return NULL;
}

static jig_handler main_thread_error_handler = {
    .method = "test.mainthread.err",
    .thread_target = JIG_THREAD_MAIN,
    .handle = main_thread_error_handle,
    .user_data = NULL,
};

/* --- rejecting middleware --- */
static int reject_all(void *ctx, const char *method, cJSON *params,
                       jig_session *session, jig_error **err) {
    (void)ctx; (void)method; (void)params; (void)session;
    *err = jig_error_handshake_required();
    return 1;
}

/* --- tests --- */
static jig_app_info *make_app(void) {
    return jig_app_info_create("TestApp", "com.test.app", "ios", "0.74.0", NULL);
}

static void test_dispatch_valid_command(void) {
    clear_sent();
    jig_handler *handlers[] = { &echo_handler };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);
    sess->session_id = strdup("sess_test1234");

    const char *msg = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"test.echo\","
                      "\"params\":{\"hello\":\"world\"}}";
    jig_dispatcher_dispatch(cfg, msg, sess);

    ASSERT(last_sent != NULL, "dispatch valid command: response sent");

    cJSON *resp = cJSON_Parse(last_sent);
    ASSERT(resp != NULL, "dispatch valid command: valid JSON response");
    ASSERT(cJSON_GetObjectItem(resp, "result") != NULL,
           "dispatch valid command: has result field");
    cJSON *id = cJSON_GetObjectItem(resp, "id");
    ASSERT(id != NULL && id->valueint == 1,
           "dispatch valid command: id matches");
    cJSON *result = cJSON_GetObjectItem(resp, "result");
    cJSON *hello = cJSON_GetObjectItem(result, "hello");
    ASSERT(hello != NULL && strcmp(hello->valuestring, "world") == 0,
           "dispatch valid command: result echoed params");
    cJSON_Delete(resp);

    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_dispatcher_destroy(cfg);
}

static void test_dispatch_unknown_method(void) {
    clear_sent();
    jig_handler *handlers[] = { &echo_handler };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);
    sess->session_id = strdup("sess_test1234");

    const char *msg = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"unknown.method\"}";
    jig_dispatcher_dispatch(cfg, msg, sess);

    ASSERT(last_sent != NULL, "unknown method: response sent");
    cJSON *resp = cJSON_Parse(last_sent);
    cJSON *err = cJSON_GetObjectItem(resp, "error");
    ASSERT(err != NULL, "unknown method: has error field");
    cJSON *code = cJSON_GetObjectItem(err, "code");
    ASSERT(code != NULL && code->valueint == JIG_ERROR_METHOD_NOT_FOUND,
           "unknown method: correct error code");
    cJSON_Delete(resp);

    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_dispatcher_destroy(cfg);
}

static void test_dispatch_invalid_json(void) {
    clear_sent();
    jig_handler *handlers[] = { &echo_handler };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    jig_dispatcher_dispatch(cfg, "not json at all{{{", sess);

    ASSERT(last_sent != NULL, "invalid json: response sent");
    cJSON *resp = cJSON_Parse(last_sent);
    cJSON *err = cJSON_GetObjectItem(resp, "error");
    ASSERT(err != NULL, "invalid json: has error");
    cJSON *code = cJSON_GetObjectItem(err, "code");
    ASSERT(code != NULL && code->valueint == JIG_ERROR_PARSE_ERROR,
           "invalid json: parse error code");
    /* id should be null for parse errors */
    cJSON *id = cJSON_GetObjectItem(resp, "id");
    ASSERT(id != NULL && cJSON_IsNull(id), "invalid json: id is null");
    cJSON_Delete(resp);

    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_dispatcher_destroy(cfg);
}

static void test_dispatch_missing_jsonrpc(void) {
    clear_sent();
    jig_handler *handlers[] = { &echo_handler };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    const char *msg = "{\"id\":3,\"method\":\"test.echo\"}";
    jig_dispatcher_dispatch(cfg, msg, sess);

    ASSERT(last_sent != NULL, "missing jsonrpc: response sent");
    cJSON *resp = cJSON_Parse(last_sent);
    cJSON *err = cJSON_GetObjectItem(resp, "error");
    cJSON *code = cJSON_GetObjectItem(err, "code");
    ASSERT(code != NULL && code->valueint == JIG_ERROR_INVALID_REQUEST,
           "missing jsonrpc: invalid request error code");
    cJSON_Delete(resp);

    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_dispatcher_destroy(cfg);
}

static void test_dispatch_middleware_rejection(void) {
    clear_sent();
    jig_middleware mws[1];
    mws[0].validate = reject_all;
    mws[0].ctx = NULL;

    jig_handler *handlers[] = { &echo_handler };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        mws, 1, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);
    sess->session_id = strdup("sess_test1234");

    const char *msg = "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"test.echo\"}";
    jig_dispatcher_dispatch(cfg, msg, sess);

    ASSERT(last_sent != NULL, "middleware rejection: response sent");
    cJSON *resp = cJSON_Parse(last_sent);
    cJSON *err = cJSON_GetObjectItem(resp, "error");
    ASSERT(err != NULL, "middleware rejection: has error");
    cJSON *code = cJSON_GetObjectItem(err, "code");
    ASSERT(code != NULL && code->valueint == JIG_ERROR_HANDSHAKE_REQUIRED,
           "middleware rejection: correct error code");
    cJSON_Delete(resp);

    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_dispatcher_destroy(cfg);
}

static void test_dispatch_notification_no_response_on_error(void) {
    clear_sent();
    jig_handler *handlers[] = { &echo_handler };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    /* notification: no id field, unknown method */
    const char *msg = "{\"jsonrpc\":\"2.0\",\"method\":\"unknown.notify\"}";
    jig_dispatcher_dispatch(cfg, msg, sess);

    ASSERT(last_sent == NULL,
           "notification unknown method: no response sent");

    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_dispatcher_destroy(cfg);
}

static void test_handle_open_sends_server_hello(void) {
    clear_sent();
    jig_handler *hs = jig_handshake_create();
    jig_handler *handlers[] = { hs, &echo_handler };
    const char *domains[] = { "network", "console" };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 2, domains, 2);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    jig_dispatcher_handle_open(cfg, sess);

    ASSERT(last_sent != NULL, "handle_open: message sent");

    cJSON *msg = cJSON_Parse(last_sent);
    ASSERT(msg != NULL, "handle_open: valid JSON");

    cJSON *method = cJSON_GetObjectItem(msg, "method");
    ASSERT(method != NULL && strcmp(method->valuestring, "server.hello") == 0,
           "handle_open: method is server.hello");

    /* Should NOT have an id (it's a notification from server) */
    ASSERT(cJSON_GetObjectItem(msg, "id") == NULL,
           "handle_open: no id field");

    cJSON *params = cJSON_GetObjectItem(msg, "params");
    ASSERT(params != NULL, "handle_open: has params");

    cJSON *proto = cJSON_GetObjectItem(params, "protocol");
    ASSERT(proto != NULL, "handle_open: has protocol");
    cJSON *pname = cJSON_GetObjectItem(proto, "name");
    ASSERT(pname != NULL && strcmp(pname->valuestring, "jig") == 0,
           "handle_open: protocol name is jig");

    cJSON *commands = cJSON_GetObjectItem(params, "commands");
    ASSERT(commands != NULL && cJSON_IsArray(commands),
           "handle_open: has commands array");
    /* Should contain test.echo but NOT client.hello */
    int found_echo = 0, found_hello = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, commands) {
        if (strcmp(item->valuestring, "test.echo") == 0) found_echo = 1;
        if (strcmp(item->valuestring, "client.hello") == 0) found_hello = 1;
    }
    ASSERT(found_echo, "handle_open: commands includes test.echo");
    ASSERT(!found_hello, "handle_open: commands excludes client.hello");

    cJSON *doms = cJSON_GetObjectItem(params, "domains");
    ASSERT(doms != NULL && cJSON_GetArraySize(doms) == 2,
           "handle_open: has 2 domains");

    cJSON *appj = cJSON_GetObjectItem(params, "app");
    ASSERT(appj != NULL, "handle_open: has app info");
    cJSON *appname = cJSON_GetObjectItem(appj, "name");
    ASSERT(appname != NULL && strcmp(appname->valuestring, "TestApp") == 0,
           "handle_open: app name matches");

    cJSON_Delete(msg);
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_handshake_destroy(hs);
    jig_dispatcher_destroy(cfg);
}

static void test_dispatch_client_hello_full(void) {
    clear_sent();
    jig_handler *hs = jig_handshake_create();
    jig_handler *handlers[] = { hs };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    const char *msg =
        "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"client.hello\","
        "\"params\":{\"protocol\":{\"version\":1},\"client\":\"test/1.0\"}}";
    jig_dispatcher_dispatch(cfg, msg, sess);

    ASSERT(last_sent != NULL, "client.hello full: response sent");

    cJSON *resp = cJSON_Parse(last_sent);
    ASSERT(resp != NULL, "client.hello full: valid JSON");

    cJSON *result = cJSON_GetObjectItem(resp, "result");
    ASSERT(result != NULL, "client.hello full: has result");

    /* Response must contain sessionId, negotiated, enabled, rejected */
    cJSON *sid = cJSON_GetObjectItem(result, "sessionId");
    ASSERT(sid != NULL && cJSON_IsString(sid),
           "client.hello full: has sessionId string");

    cJSON *negotiated = cJSON_GetObjectItem(result, "negotiated");
    ASSERT(negotiated != NULL && cJSON_IsObject(negotiated),
           "client.hello full: has negotiated object");
    cJSON *neg_proto = cJSON_GetObjectItem(negotiated, "protocol");
    ASSERT(neg_proto != NULL && neg_proto->valueint == 1,
           "client.hello full: negotiated protocol is 1");

    cJSON *enabled = cJSON_GetObjectItem(result, "enabled");
    ASSERT(enabled != NULL && cJSON_IsArray(enabled),
           "client.hello full: has enabled array");

    cJSON *rejected = cJSON_GetObjectItem(result, "rejected");
    ASSERT(rejected != NULL && cJSON_IsArray(rejected),
           "client.hello full: has rejected array");

    /* Session state must be updated by dispatcher post-processing */
    ASSERT(sess->session_id != NULL,
           "client.hello full: session_id set after dispatch");
    ASSERT(strcmp(sess->session_id, sid->valuestring) == 0,
           "client.hello full: session_id matches response");
    ASSERT(sess->negotiated_version == 1,
           "client.hello full: negotiated_version is 1");

    cJSON_Delete(resp);
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_handshake_destroy(hs);
    jig_dispatcher_destroy(cfg);
}

static void test_dispatch_main_thread_handler(void) {
    clear_sent();
    g_captured.callback = NULL;
    g_captured.ctx = NULL;

    static jig_platform_ops ops = {
        .screenshot = NULL,
        .get_app_info = NULL,
        .log = NULL,
        .run_on_main_thread = mock_run_on_main_thread,
    };
    jig_platform_set_ops(&ops);

    jig_handler *handlers[] = { &main_thread_handler };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);
    sess->session_id = strdup("sess_test1234");

    const char *msg = "{\"jsonrpc\":\"2.0\",\"id\":99,\"method\":\"test.mainthread\","
                      "\"params\":{\"key\":\"value\"}}";
    jig_dispatcher_dispatch(cfg, msg, sess);

    ASSERT(last_sent == NULL,
           "main_thread dispatch: no immediate response");
    ASSERT(g_captured.callback != NULL,
           "main_thread dispatch: callback was captured");

    flush_main_thread();

    ASSERT(last_sent != NULL,
           "main_thread dispatch: response sent after main thread flush");

    cJSON *resp = cJSON_Parse(last_sent);
    ASSERT(resp != NULL, "main_thread dispatch: valid JSON response");
    cJSON *id = cJSON_GetObjectItem(resp, "id");
    ASSERT(id != NULL && id->valueint == 99,
           "main_thread dispatch: id matches");
    cJSON *result = cJSON_GetObjectItem(resp, "result");
    cJSON *dispatched = cJSON_GetObjectItem(result, "dispatched");
    ASSERT(dispatched != NULL && strcmp(dispatched->valuestring, "on_main") == 0,
           "main_thread dispatch: handler result correct");
    cJSON_Delete(resp);

    jig_platform_set_ops(NULL);
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_dispatcher_destroy(cfg);
}

static void test_dispatch_main_thread_error(void) {
    clear_sent();
    g_captured.callback = NULL;
    g_captured.ctx = NULL;

    static jig_platform_ops ops = {
        .screenshot = NULL,
        .get_app_info = NULL,
        .log = NULL,
        .run_on_main_thread = mock_run_on_main_thread,
    };
    jig_platform_set_ops(&ops);

    jig_handler *handlers[] = { &main_thread_error_handler };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);
    sess->session_id = strdup("sess_test1234");

    const char *msg = "{\"jsonrpc\":\"2.0\",\"id\":100,\"method\":\"test.mainthread.err\"}";
    jig_dispatcher_dispatch(cfg, msg, sess);

    ASSERT(last_sent == NULL, "main_thread error: no immediate response");
    flush_main_thread();

    ASSERT(last_sent != NULL, "main_thread error: response sent after flush");
    cJSON *resp = cJSON_Parse(last_sent);
    cJSON *err = cJSON_GetObjectItem(resp, "error");
    ASSERT(err != NULL, "main_thread error: has error field");
    cJSON *code = cJSON_GetObjectItem(err, "code");
    ASSERT(code != NULL && code->valueint == JIG_ERROR_INTERNAL_ERROR,
           "main_thread error: correct error code");
    cJSON_Delete(resp);

    jig_platform_set_ops(NULL);
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_dispatcher_destroy(cfg);
}

static void test_dispatch_main_thread_no_platform_op(void) {
    clear_sent();

    static jig_platform_ops ops = {
        .screenshot = NULL,
        .get_app_info = NULL,
        .log = NULL,
        .run_on_main_thread = NULL,
    };
    jig_platform_set_ops(&ops);

    jig_handler *handlers[] = { &main_thread_handler };
    jig_dispatcher_config *cfg = jig_dispatcher_create(
        NULL, 0, handlers, 1, NULL, 0);

    jig_app_info *app = make_app();
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);
    sess->session_id = strdup("sess_test1234");

    const char *msg = "{\"jsonrpc\":\"2.0\",\"id\":101,\"method\":\"test.mainthread\"}";
    jig_dispatcher_dispatch(cfg, msg, sess);

    ASSERT(last_sent != NULL,
           "no platform op: response sent synchronously");

    cJSON *resp = cJSON_Parse(last_sent);
    cJSON *result = cJSON_GetObjectItem(resp, "result");
    cJSON *dispatched = cJSON_GetObjectItem(result, "dispatched");
    ASSERT(dispatched != NULL && strcmp(dispatched->valuestring, "on_main") == 0,
           "no platform op: handler result correct");
    cJSON_Delete(resp);

    jig_platform_set_ops(NULL);
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_dispatcher_destroy(cfg);
}

int main(void) {
    test_dispatch_valid_command();
    test_dispatch_unknown_method();
    test_dispatch_invalid_json();
    test_dispatch_missing_jsonrpc();
    test_dispatch_middleware_rejection();
    test_dispatch_notification_no_response_on_error();
    test_handle_open_sends_server_hello();
    test_dispatch_client_hello_full();
    test_dispatch_main_thread_handler();
    test_dispatch_main_thread_error();
    test_dispatch_main_thread_no_platform_op();

    clear_sent();

    if (failures == 0) {
        printf("test_dispatcher: all tests passed\n");
    } else {
        printf("test_dispatcher: %d failure(s)\n", failures);
    }
    return failures;
}
