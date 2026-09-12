#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <stdlib.h>

static int g_test_failures;

#define TEST_ASSERT(cond)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            g_test_failures++;                                                 \
        }                                                                      \
    } while (0)

#define TEST_RUN(fn)                                                           \
    do {                                                                       \
        int before = g_test_failures;                                          \
        fn();                                                                  \
        if (g_test_failures == before)                                         \
            printf("ok  %s\n", #fn);                                           \
        else                                                                   \
            printf("FAIL %s\n", #fn);                                          \
    } while (0)

static int test_finish(void) {
    if (g_test_failures) {
        fprintf(stderr, "%d assertion(s) failed\n", g_test_failures);
        return 1;
    }
    printf("All tests passed\n");
    return 0;
}

#endif
