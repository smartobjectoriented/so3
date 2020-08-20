#include <stdio.h>

#ifndef __TEST_H_SO3__
#define __TEST_H_SO3__

#define SO3_TEST_SUCCESS(fmt, ...) \
    do { \
        printf("[SO3 TEST [PASS]] file : %s function : %s\n", __FILE__, __PRETTY_FUNCTION__); \
        printf(fmt, ## __VA_ARGS__); \
    } while(0)

#define SO3_TEST_FAIL(fmt, ...) \
    do { \
        printf("[SO3 TEST [FAIL]] file : %s function : %s\n", __FILE__, __PRETTY_FUNCTION__); \
        printf(fmt, ## __VA_ARGS__); \
    } while(0)
    
#endif // __TEST_H_SO3__
