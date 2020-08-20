#include <stdlib.h>
#include <stdio.h>

#define MAX (1024*1024)

static void *ptr[MAX] = {NULL,}; // OK

int main()
{
    printf("Before array of pointers\n");

    // This also fails on the host (with a segmentation fault)
    // Probably the stack size is not big enough to hold this
    //void *ptr[MAX] = {NULL,}; // -> Kernel panic !
    //void *ptr[MAX]; // -> Kernel panic !

    printf("Array of pointers set\n");
    
    for (int i = 0; i < MAX; ++i) {
        ptr[i] = (void *)0xdeadbeef;
    }

    printf("Array of pointers beefed\n");
    
    for (int i = 0; i < MAX; ++i) {
        ptr[i] = (void *)~(unsigned int)ptr[i];
    }

    printf("Array of pointers pointed, will return 0\n");
        
    return 0;
}
