#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int main()
{
    void *ptr = malloc(100);

    //ASSERT_TRUE(ptr != nullptr);
    if (ptr == NULL) {
        printf("[FAIL] malloc() returned a nullptr\n");
        return -1;
    }
    //ASSERT_LE(100U, malloc_usable_size(ptr));
//    if (malloc_usable_size(ptr) < 100) {
//        printf("[FAIL] malloc() usable size too small\n");
//        return -1;
//    }

    free(ptr);
    
    printf("[PASS] malloc() test 01 did pass\n");
    return 0;
}
