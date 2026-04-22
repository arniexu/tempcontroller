#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>

#define TEST_ASSERT_TRUE(expr)                                                         \
    do                                                                                 \
    {                                                                                  \
        if (!(expr))                                                                   \
        {                                                                              \
            fprintf(stderr, "ASSERT_TRUE failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
            return 1;                                                                  \
        }                                                                              \
    } while (0)

#define TEST_ASSERT_EQ_INT(actual, expected)                                           \
    do                                                                                 \
    {                                                                                  \
        if ((actual) != (expected))                                                   \
        {                                                                              \
            fprintf(stderr,                                                            \
                    "ASSERT_EQ_INT failed at %s:%d: actual=%d expected=%d\n",       \
                    __FILE__,                                                          \
                    __LINE__,                                                          \
                    (int)(actual),                                                     \
                    (int)(expected));                                                  \
            return 1;                                                                  \
        }                                                                              \
    } while (0)

#define TEST_ASSERT_NEAR_FLOAT(actual, expected, eps)                                  \
    do                                                                                 \
    {                                                                                  \
        float _a = (float)(actual);                                                    \
        float _e = (float)(expected);                                                  \
        float _d = _a - _e;                                                            \
        if (_d < 0.0f)                                                                 \
        {                                                                              \
            _d = -_d;                                                                  \
        }                                                                              \
        if (_d > (eps))                                                                \
        {                                                                              \
            fprintf(stderr,                                                            \
                    "ASSERT_NEAR_FLOAT failed at %s:%d: actual=%f expected=%f eps=%f\n", \
                    __FILE__,                                                          \
                    __LINE__,                                                          \
                    (double)_a,                                                        \
                    (double)_e,                                                        \
                    (double)(eps));                                                    \
            return 1;                                                                  \
        }                                                                              \
    } while (0)

#endif
