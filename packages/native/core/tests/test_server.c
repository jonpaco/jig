#include "test_util.h"
#include "jig_server.h"
#include "jig_dispatcher.h"
#include "jig_app_info.h"

static void test_create_destroy(void) {
    jig_app_info *app = jig_app_info_create("TestApp", "com.test.app", "ios", "0.75.0", NULL);
    ASSERT(app != NULL, "app_info created");

    jig_dispatcher_config *dispatcher = jig_dispatcher_create(NULL, 0, NULL, 0, NULL, 0);
    ASSERT(dispatcher != NULL, "dispatcher created");

    /* Create server on a high port to avoid conflicts */
    jig_server *server = jig_server_create(14042, dispatcher, app);
    ASSERT(server != NULL, "server created successfully");

    /* Destroy without starting -- verifies cleanup path */
    jig_server_destroy(server);

    jig_dispatcher_destroy(dispatcher);
    jig_app_info_free(app);
}

static void test_create_null_args(void) {
    /* NULL dispatcher and app_info -- server should still create
     * (it stores pointers, doesn't dereference at create time) */
    jig_server *server = jig_server_create(14043, NULL, NULL);
    ASSERT(server != NULL, "server with NULL args created");
    jig_server_destroy(server);
}

static void test_destroy_null(void) {
    /* Should not crash */
    jig_server_destroy(NULL);
}

static void test_stop_without_start(void) {
    jig_app_info *app = jig_app_info_create("TestApp", "com.test.app", "ios", "0.75.0", NULL);
    jig_dispatcher_config *dispatcher = jig_dispatcher_create(NULL, 0, NULL, 0, NULL, 0);
    jig_server *server = jig_server_create(14044, dispatcher, app);
    ASSERT(server != NULL, "server created for stop test");

    /* Stop without start -- should not crash */
    jig_server_stop(server);

    jig_server_destroy(server);
    jig_dispatcher_destroy(dispatcher);
    jig_app_info_free(app);
}

int main(void) {
    test_create_destroy();
    test_create_null_args();
    test_destroy_null();
    test_stop_without_start();

    if (failures > 0) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    printf("All server tests passed\n");
    return 0;
}
