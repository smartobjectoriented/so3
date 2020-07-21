#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>

#define BLOCK_SIZE 333
#define MAX_ALLOCATIONS 1024*100

int main()
{
    void *ptr[MAX_ALLOCATIONS] = {NULL,};

    int i = 0;
    int ret = 0;
    
    for (i = 0; i < MAX_ALLOCATIONS; ++i) {
        ptr[i] = malloc(BLOCK_SIZE);
        if (ptr[i] == NULL) {
            printf("[FAIL] Failed to allocate block number %d of %d bytes\n", i, BLOCK_SIZE);
            ret = -1;
            break;
        }
    }

    while(i) {
        free(ptr[i-1]);
        i--;
    }

    if (ret == 0) {
        printf("[PASS]\n");
    }
    
    return ret;
}
