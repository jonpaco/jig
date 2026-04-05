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

    if (failures > 0) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All platform tests passed.\n");
    return 0;
}
