#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"
#define NUM_THREADS     5

#define RETURN_VALUE ((void *)0xdeadc0de)

void *PrintHello(void *threadid)
{
    long tid;
    tid = (long)threadid;
    printf("Hello World! It's me, thread #%ld!\n", tid);
    pthread_exit(RETURN_VALUE);

    return RETURN_VALUE; // For signature
}

int main (int argc, char *argv[])
{
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;
    for(t = 0; t < NUM_THREADS; t++) {
        printf("In main: creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
        if (rc) {
            //printf("ERROR; return code from pthread_create() is %d\n", rc);
            SO3_TEST_FAIL("return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for(t = 0; t < NUM_THREADS; t++) {
        void *ret;
        // https://stackoverflow.com/questions/47222124/why-the-second-argument-to-pthread-join-is-a-a-pointer-to-a-pointer
        rc = pthread_join(threads[t], &ret);
        if (rc) {
            //printf("ERROR; return code from pthread_join() is %d\n", rc);
            SO3_TEST_FAIL("return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
        if (ret != RETURN_VALUE) {
            //printf("ERROR; thread did not return correct value\nExpected : 0x%08x Received : 0x%08x\n", RETURN_VALUE, ret);
            SO3_TEST_FAIL("thread did not return correct value\nExpected : 0x%08x Received : 0x%08x\n", RETURN_VALUE, ret);
            exit(-1);
        } else {
            printf("Thread %d correctly returned : 0x%08x\n", t, ret);
        }
    }

    SO3_TEST_SUCCESS("thread test did pass correctly\n");
    return 0;
}
