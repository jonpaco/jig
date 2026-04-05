#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdio.h>

static int failures = 0;

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); \
        failures++; \
    } \
} while (0)

#endif /* TEST_UTIL_H */
