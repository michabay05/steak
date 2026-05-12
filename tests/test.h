// mabay: Inspired by https://github.com/erincatto/box2d/blob/8c65dcb91a5e8fbce53492c169cb4460b53b0b54/test/test_macros.h

#ifndef _TEST_H_
#define _TEST_H_

#define ENSURE(cond) \
    do { \
        if ((cond) == 0) { \
            fprintf(stderr, \
                "[FAIL] %s:%d - '" #cond "'\n", __FUNCTION__, __LINE__); \
            __builtin_trap(); \
        } \
    } while (0) \

#endif // _TEST_H_
