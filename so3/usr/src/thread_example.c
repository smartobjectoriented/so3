/*
 * Copyright (C) 2025 Clément Dieperink <clement.dieperink@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* Example demonstrating POSIX threads on SO3: spawn worker threads that
 * contend on a shared pthread mutex. */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

struct thread_args {
	long thread_id;
	pthread_mutex_t *mutex;
};

void *simple_thread(void *ptr)
{
	struct thread_args *shared_ptr = ptr;

	printf("Thread %ld: Executing\n", shared_ptr->thread_id);

	pthread_mutex_lock(shared_ptr->mutex);
	printf("Thread %ld: Locked mutex\n", shared_ptr->thread_id);
	sleep(3);
	printf("Thread %ld: Unlocking mutex\n", shared_ptr->thread_id);
	pthread_mutex_unlock(shared_ptr->mutex);

	printf("Thread %ld: Thread finished\n", shared_ptr->thread_id);
	/* Return id to check if the value is correctly retrieved by main thread */
	return (void *) shared_ptr->thread_id;
}

int main(int argc, char **argv)
{
	int ret;
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

	long thread_ret[2];

	/* Start the threads */
	ret = pthread_create(&thread[0], NULL, *simple_thread, (void *) &args[0]);
	if (ret != 0) {
		printf("Could not create thread 1\n");
	}

	ret = pthread_create(&thread[1], NULL, *simple_thread, (void *) &args[1]);
	if (ret != 0) {
		printf("Could not create thread 2\n");
	}

	/* Wait until thread are finished. */
	pthread_join(thread[0], (void **) &thread_ret[0]);
	pthread_join(thread[1], (void **) &thread_ret[1]);

	/* Simple test to see if thread return value is retrieved correctly. */
	if (thread_ret[0] != 1) {
		printf("Unexpected return value for thread 1\n");
	}
	if (thread_ret[1] != 2) {
		printf("Unexpected return value for thread 2\n");
	}

	printf("All threads finished\n");
	return 0;
}
