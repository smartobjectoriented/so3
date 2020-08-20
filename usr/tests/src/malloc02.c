#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>

#include "test.h"

int main()
{
    void *ptr = malloc(INT_MAX);

    //ASSERT_TRUE(ptr != nullptr);
    if (ptr == NULL) {
        //printf("[PASS] malloc() returned a nullptr\n");
        SO3_TEST_SUCCESS("malloc() returned a nullptr\n");
        return 0;
    } else {
        //ASSERT_LE(100U, malloc_usable_size(ptr));
        //    if (malloc_usable_size(ptr) < 100) {
        //        printf("[FAIL] malloc() usable size too small\n");
        //        return -1;
        //    }

        free(ptr);
    
        //printf("[FAIL] malloc() allocated an osbscene amount of bytes\n");
        SO3_TEST_FAIL("malloc() allocated an osbscene amount of bytes\n");
        return -1;
    }
}
