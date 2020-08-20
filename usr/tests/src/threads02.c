#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#define NUM_THREADS     50
#define UNLIMITED       0

void *threaded_fn(void *param) {
    unsigned int num = (unsigned int) param;
    pthread_t thread;

    if (UNLIMITED || num < NUM_THREADS) {
        printf("Creating thread %d\n", num + 1);
        int rc = pthread_create(&thread, NULL, threaded_fn, (void *) num + 1);

        if (rc) {
            if (UNLIMITED) {
                printf("Testing pthread limit, %d threads created\n", num);
                pthread_exit(NULL);
            }
            if (rc == EAGAIN) {
                printf("Thread %d : Unable to create more threads\n", num);
                return NULL;
            } else {
                printf("[ERROR] Thread %d : pthread_create() failed\n", num);
                return NULL;
            }
        }
        pthread_join(thread, NULL);
    }
    
    return NULL;
}

int main (int argc, char **argv)
{
    if (UNLIMITED) {
        printf("Creating as many threads as possible\n");
    } else {
        printf("Creating %d threads\n", NUM_THREADS);
    }

    threaded_fn(0);

    printf("Finished\n");
    
    return 0;
}
