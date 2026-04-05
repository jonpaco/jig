#include <stdlib.h>
#include "../jig_platform.h"
#include "test_util.h"

static int mock_screenshot(jig_screenshot_opts *opts, jig_screenshot_result *result) {
    (void)opts; (void)result;
    return 0;
}

static int mock_get_app_info(jig_app_info *info) {
    (void)info;
    return 0;
}

static void mock_log(const char *fmt, ...) {
    (void)fmt;
}

static void test_get_ops_returns_null_before_set(void) {
    const jig_platform_ops *ops = jig_platform_get_ops();
    ASSERT(ops == NULL, "get_ops should return NULL before any set");
}

static int main_thread_called = 0;
static void mock_main_thread_callback(void *ctx) {
    int *flag = (int *)ctx;
    *flag = 42;
    main_thread_called = 1;
}

static void mock_run_on_main_thread(void (*callback)(void *ctx), void *ctx) {
    /* In tests, just call it synchronously */
    callback(ctx);
}

static void test_platform_run_on_main_thread(void) {
    static jig_platform_ops ops = {
        .screenshot = NULL,
        .get_app_info = NULL,
        .log = NULL,
        .run_on_main_thread = mock_run_on_main_thread,
    };
    jig_platform_set_ops(&ops);

    const jig_platform_ops *retrieved = jig_platform_get_ops();
    ASSERT(retrieved->run_on_main_thread != NULL,
           "run_on_main_thread: field is set");

    int value = 0;
    retrieved->run_on_main_thread(mock_main_thread_callback, &value);
    ASSERT(value == 42, "run_on_main_thread: callback executed with context");
    ASSERT(main_thread_called == 1, "run_on_main_thread: callback was called");

    jig_platform_set_ops(NULL);
}

static void test_set_then_get_roundtrip(void) {
    jig_platform_ops my_ops = {
        .screenshot = mock_screenshot,
        .get_app_info = mock_get_app_info,
        .log = mock_log,
    };

    jig_platform_set_ops(&my_ops);
    const jig_platform_ops *got = jig_platform_get_ops();

    ASSERT(got != NULL, "get_ops should not be NULL after set");
    ASSERT(got == &my_ops, "get_ops should return the same pointer that was set");
    ASSERT(got->screenshot == mock_screenshot, "screenshot fn pointer should match");
    ASSERT(got->get_app_info == mock_get_app_info, "get_app_info fn pointer should match");
    ASSERT(got->log == mock_log, "log fn pointer should match");

    /* reset for other tests */
    jig_platform_set_ops(NULL);
}

int main(void) {
    test_get_ops_returns_null_before_set();
    test_set_then_get_roundtrip();
    test_platform_run_on_main_thread();

    if (failures > 0) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All platform tests passed.\n");
    return 0;
}
