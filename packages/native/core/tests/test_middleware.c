#include "test_util.h"
#include "../middleware/jig_handshake_gate.h"
#include "../middleware/jig_domain_guard.h"
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

/* --- Handshake gate tests --- */

static void test_gate_allows_client_hello(void) {
    clear_sent();
    jig_middleware gate = jig_handshake_gate_create();
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74.0", NULL);
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);
    /* session_id is NULL (pre-handshake) */

    jig_error *err = NULL;
    int rc = gate.validate(gate.ctx, "client.hello", NULL, sess, &err);

    ASSERT(rc == 0, "gate: allows client.hello pre-handshake");
    ASSERT(err == NULL, "gate: no error for client.hello");

    jig_session_destroy(sess);
    jig_app_info_free(app);
}

static void test_gate_rejects_pre_handshake(void) {
    clear_sent();
    jig_middleware gate = jig_handshake_gate_create();
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74.0", NULL);
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);
    /* session_id is NULL */

    jig_error *err = NULL;
    int rc = gate.validate(gate.ctx, "view.getTree", NULL, sess, &err);

    ASSERT(rc != 0, "gate: rejects pre-handshake command");
    ASSERT(err != NULL, "gate: has error");
    ASSERT(err->code == JIG_ERROR_HANDSHAKE_REQUIRED,
           "gate: handshake required error code");

    jig_error_free(err);
    jig_session_destroy(sess);
    jig_app_info_free(app);
}

static void test_gate_allows_post_handshake(void) {
    clear_sent();
    jig_middleware gate = jig_handshake_gate_create();
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74.0", NULL);
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);
    sess->session_id = strdup("sess_abcd1234");

    jig_error *err = NULL;
    int rc = gate.validate(gate.ctx, "view.getTree", NULL, sess, &err);

    ASSERT(rc == 0, "gate: allows post-handshake command");
    ASSERT(err == NULL, "gate: no error post-handshake");

    jig_session_destroy(sess);
    jig_app_info_free(app);
}

/* --- Domain guard tests --- */

static void test_guard_allows_non_domain_methods(void) {
    clear_sent();
    const char *domains[] = { "network" };
    jig_middleware guard = jig_domain_guard_create(domains, 1);
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74.0", NULL);
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    jig_error *err = NULL;
    int rc = guard.validate(guard.ctx, "view.getTree", NULL, sess, &err);

    ASSERT(rc == 0, "domain guard: allows non-domain method");
    ASSERT(err == NULL, "domain guard: no error for non-domain");

    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_domain_guard_destroy(&guard);
}

static void test_guard_rejects_unsupported_domain(void) {
    clear_sent();
    const char *domains[] = { "network" };
    jig_middleware guard = jig_domain_guard_create(domains, 1);
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74.0", NULL);
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    jig_error *err = NULL;
    int rc = guard.validate(guard.ctx, "console.enable", NULL, sess, &err);

    ASSERT(rc != 0, "domain guard: rejects unsupported domain");
    ASSERT(err != NULL, "domain guard: has error");
    ASSERT(err->code == JIG_ERROR_UNSUPPORTED_DOMAIN,
           "domain guard: unsupported domain error code");

    jig_error_free(err);
    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_domain_guard_destroy(&guard);
}

static void test_guard_allows_supported_domain(void) {
    clear_sent();
    const char *domains[] = { "network", "console" };
    jig_middleware guard = jig_domain_guard_create(domains, 2);
    jig_app_info *app = jig_app_info_create("Test", "com.test", "ios", "0.74.0", NULL);
    jig_session *sess = jig_session_create(1, app, capture_send, NULL);

    jig_error *err = NULL;
    int rc = guard.validate(guard.ctx, "network.enable", NULL, sess, &err);

    ASSERT(rc == 0, "domain guard: allows supported domain");
    ASSERT(err == NULL, "domain guard: no error for supported domain");

    jig_session_destroy(sess);
    jig_app_info_free(app);
    jig_domain_guard_destroy(&guard);
}

int main(void) {
    test_gate_allows_client_hello();
    test_gate_rejects_pre_handshake();
    test_gate_allows_post_handshake();
    test_guard_allows_non_domain_methods();
    test_guard_rejects_unsupported_domain();
    test_guard_allows_supported_domain();

    clear_sent();

    if (failures == 0) {
        printf("test_middleware: all tests passed\n");
    } else {
        printf("test_middleware: %d failure(s)\n", failures);
    }
    return failures;
}
