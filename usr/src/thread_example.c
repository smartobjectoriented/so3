#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

struct thread_args {
	int thread_id;
	pthread_mutex_t *mutex;
};

void *simple_thread(void *ptr)
{
	struct thread_args *shared_ptr = ptr;

	printf("Thread %d: Executing\n", shared_ptr->thread_id);

	pthread_mutex_lock(shared_ptr->mutex);
	printf("Thread %d: Locked mutex\n", shared_ptr->thread_id);
	sleep(3);
	printf("Thread %d: Unlocking mutex\n", shared_ptr->thread_id);
	pthread_mutex_unlock(shared_ptr->mutex);

	printf("Thread %d: Thread finished\n", shared_ptr->thread_id);
	return shared_ptr;
}

int main(int argc, char **argv)
{
	pthread_t thread[2];
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	struct thread_args args[2] = {
		{
			.thread_id = 1,
			.mutex = &mutex,
		},
		{
			.thread_id = 2,
			.mutex = &mutex,
		},
	};

	struct thread_args *ret[2];

	/* Start the threads */
	pthread_create(&thread[0], NULL, *simple_thread, (void *) &args[0]);
	pthread_create(&thread[1], NULL, *simple_thread, (void *) &args[1]);

	/* Wait until thread are finished. */
	pthread_join(thread[0], (void **)&ret[0]);
	pthread_join(thread[1], (void **)&ret[1]);

	/* Simple test to see if thread return value is retrieved correctly. */
	if (ret[0] != &args[0]) {
		printf("Unexpected return value for thread 1\n");
	}
	if (ret[1] != &args[1]) {
		printf("Unexpected return value for thread 2\n");
	}

	printf("All threads finished\n");
	return 0;
}
