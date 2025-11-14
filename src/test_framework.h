/* test_framework.h */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

#define TEST_COLOR_RED   "\x1b[31m"
#define TEST_COLOR_GREEN "\x1b[32m"
#define TEST_COLOR_YELLOW "\x1b[33m"
#define TEST_COLOR_RESET "\x1b[0m"

typedef struct test_context_t {
    int total_test_count;
    int current_test_index;
    int current_step_index;
    int total_failure_count;
    int total_success_count;
} test_context_t;

static test_context_t test_context = {
    .total_test_count = 0,
    .current_test_index = 0,
    .current_step_index = 0,
    .total_failure_count = 0,
    .total_success_count = 0,
};

#define TEST_SUITE_BEGIN(total_tests)                                    \
    do {                                                                 \
        test_context.total_test_count = (total_tests);                   \
        test_context.current_test_index = 0;                             \
        test_context.current_step_index = 0;                             \
        test_context.total_failure_count = 0;                            \
        test_context.total_success_count = 0;                            \
    } while (0)

/*
 * Usage:
 *   TEST("some name") {
 *       ...
 *   }
 *
 * Must be used inside a function (e.g. main), not at global scope.
 */
#define TEST(name_literal)                                               \
    test_context.current_test_index += 1;                                \
    test_context.current_step_index = 0;                                 \
    printf("\n=== [%2d/%2d] %s(%s)%s %s[%s]%s ==========================================\n", \
           test_context.current_test_index,                              \
           test_context.total_test_count,\
           TEST_COLOR_YELLOW ,__FILE__,TEST_COLOR_RESET,                                \
           TEST_COLOR_GREEN ,(name_literal),TEST_COLOR_RESET \
        );                                              \
    for (bool _test_once = true; _test_once; _test_once = false)

/* Simple message assert */
#define ASSERT(condition, message_literal)                                   \
    do {                                                                     \
        test_context.current_step_index += 1;                                \
        int _ok = (condition);                                               \
        if (!_ok) {                                                          \
            test_context.total_failure_count += 1;                           \
        } else {\
            test_context.total_success_count += 1;\
        }                                                                 \
        const char *_status_text  = _ok ? " OK" : "NOK";                     \
        const char *_status_color = _ok ? TEST_COLOR_GREEN : TEST_COLOR_RED; \
        printf("         - [%02d.%02d] [%s%s%s] %s(line:%4d)%s %s\n",          \
               test_context.current_test_index,                              \
               test_context.current_step_index,                              \
               _status_color, _status_text, TEST_COLOR_RESET,               \
               TEST_COLOR_YELLOW, __LINE__,TEST_COLOR_RESET,                   \
               (message_literal));      \
    } while (0)

/* Formatted message assert */
#define ASSERTF(condition, format_literal, ...)                              \
    do {                                                                     \
        test_context.current_step_index += 1;                                \
        int _ok = (condition);                                               \
        if (!_ok) {                                                          \
            test_context.total_failure_count += 1;                           \
        } else {\
            test_context.total_success_count += 1;\
        }                                                                 \
        const char *_status_text  = _ok ? " OK" : "NOK";                     \
        const char *_status_color = _ok ? TEST_COLOR_GREEN : TEST_COLOR_RED; \
        printf("         - [%02d.%02d] [%s%s%s] %s(line:%4d)%s",             \
               test_context.current_test_index,                              \
               test_context.current_step_index,                              \
               _status_color, _status_text, TEST_COLOR_RESET,\
                TEST_COLOR_YELLOW, __LINE__,TEST_COLOR_RESET);              \
        printf((format_literal), __VA_ARGS__);                               \
        printf(" \n");\
    } while (0)

/* Optional: summary and process exit status */
#define TEST_SUITE_END()                                                 \
    do {                                                                 \
        if (test_context.total_failure_count == 0) {                     \
            printf("\nAll tests passed.\n");                             \
        } else {                                                         \
            printf("\nTotal test points: %2d\nTotal success: %2d\nTotal failures: %2d\n",\
                   test_context.total_success_count + test_context.total_failure_count,\
                   test_context.total_success_count,\
                   test_context.total_failure_count);                    \
        }                                                                \
    } while (0)

#endif /* TEST_FRAMEWORK_H */
