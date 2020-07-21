#include <stdlib.h>
#include <stdio.h>

#define MAX (1024*1024)

int main()
{
    printf("Before array of pointers\n");
    
    //void *ptr[MAX] = {NULL,};
    void *ptr[MAX];

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
