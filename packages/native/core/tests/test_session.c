#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../jig_session.h"

static int failures = 0;

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); \
        failures++; \
    } \
} while (0)

static char *last_sent_text = NULL;
static void *last_sent_user_data = NULL;

static void mock_send(const char *text, void *user_data) {
    free(last_sent_text);
    last_sent_text = strdup(text);
    last_sent_user_data = user_data;
}

static void test_create_destroy(void) {
    jig_app_info *info = jig_app_info_create("TestApp", "com.test", "ios", "0.76.0", NULL);
    int tag = 42;
    jig_session *s = jig_session_create(1, info, mock_send, &tag);

    ASSERT(s != NULL, "session should not be NULL");
    ASSERT(s->connection_id == 1, "connection_id should be 1");
    ASSERT(s->app_info == info, "app_info should be the same pointer");
    ASSERT(s->send_text == mock_send, "send_text should be mock_send");
    ASSERT(s->send_user_data == &tag, "send_user_data should be &tag");
    ASSERT(s->session_id == NULL, "session_id should be NULL before handshake");
    ASSERT(s->negotiated_version == 0, "negotiated_version should be 0");
    ASSERT(s->enabled_domain_count == 0, "domain count should be 0");

    jig_session_destroy(s);
    jig_app_info_free(info);
}

static void test_add_and_has_domain(void) {
    jig_app_info *info = jig_app_info_create("App", "com.app", "ios", "0.76.0", NULL);
    jig_session *s = jig_session_create(1, info, mock_send, NULL);

    ASSERT(jig_session_has_domain(s, "dom") == 0, "should not have dom initially");

    int rc = jig_session_add_domain(s, "dom");
    ASSERT(rc == 0, "add_domain should return 0 (success)");
    ASSERT(s->enabled_domain_count == 1, "domain count should be 1");
    ASSERT(jig_session_has_domain(s, "dom") == 1, "should have dom after add");

    /* adding same domain again should be a no-op */
    rc = jig_session_add_domain(s, "dom");
    ASSERT(rc == 0, "duplicate add should succeed");
    ASSERT(s->enabled_domain_count == 1, "domain count still 1 after dup");

    /* add a second domain */
    jig_session_add_domain(s, "runtime");
    ASSERT(s->enabled_domain_count == 2, "domain count should be 2");
    ASSERT(jig_session_has_domain(s, "runtime") == 1, "should have runtime");
    ASSERT(jig_session_has_domain(s, "dom") == 1, "should still have dom");

    jig_session_destroy(s);
    jig_app_info_free(info);
}

static void test_remove_domain(void) {
    jig_app_info *info = jig_app_info_create("App", "com.app", "ios", "0.76.0", NULL);
    jig_session *s = jig_session_create(1, info, mock_send, NULL);

    jig_session_add_domain(s, "dom");
    jig_session_add_domain(s, "runtime");
    ASSERT(s->enabled_domain_count == 2, "should have 2 domains");

    int rc = jig_session_remove_domain(s, "dom");
    ASSERT(rc == 0, "remove should succeed");
    ASSERT(s->enabled_domain_count == 1, "domain count should be 1 after remove");
    ASSERT(jig_session_has_domain(s, "dom") == 0, "dom should be gone");
    ASSERT(jig_session_has_domain(s, "runtime") == 1, "runtime should remain");

    /* removing non-existent domain */
    rc = jig_session_remove_domain(s, "nonexistent");
    ASSERT(rc == -1, "removing non-existent domain should return -1");

    jig_session_destroy(s);
    jig_app_info_free(info);
}

static void test_session_id_lifecycle(void) {
    jig_app_info *info = jig_app_info_create("App", "com.app", "ios", "0.76.0", NULL);
    jig_session *s = jig_session_create(1, info, mock_send, NULL);

    ASSERT(s->session_id == NULL, "session_id null initially");

    /* simulate handshake setting session_id */
    s->session_id = strdup("abc-123");
    s->negotiated_version = 1;
    ASSERT(strcmp(s->session_id, "abc-123") == 0, "session_id set");
    ASSERT(s->negotiated_version == 1, "negotiated_version set");

    /* destroy should free session_id */
    jig_session_destroy(s);
    jig_app_info_free(info);
}

static void test_send_fn(void) {
    jig_app_info *info = jig_app_info_create("App", "com.app", "ios", "0.76.0", NULL);
    int tag = 99;
    jig_session *s = jig_session_create(1, info, mock_send, &tag);

    s->send_text("hello", s->send_user_data);
    ASSERT(last_sent_text != NULL, "send should have been called");
    ASSERT(strcmp(last_sent_text, "hello") == 0, "sent text matches");
    ASSERT(last_sent_user_data == &tag, "user_data passed through");

    free(last_sent_text);
    last_sent_text = NULL;

    jig_session_destroy(s);
    jig_app_info_free(info);
}

int main(void) {
    test_create_destroy();
    test_add_and_has_domain();
    test_remove_domain();
    test_session_id_lifecycle();
    test_send_fn();

    if (failures > 0) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All session tests passed.\n");
    return 0;
}
