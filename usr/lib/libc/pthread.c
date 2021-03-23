/*
 * Copyright (C) 2014-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
 *
 */

#include <pthread.h>
#include <syscall.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

void *(*__thread_fn)(void *) = NULL;

void *__thread_routine(void *args) {
	void *(*____thread_fn)(void *);

	____thread_fn = __thread_fn;
	__thread_fn = NULL;

	____thread_fn(args);

	sys_thread_exit(NULL);

	/* We should never be here ... */

	assert(0);

	return NULL;
}

/* Yield to another thread */
int pthread_yield(void) {
	sys_thread_yield();

	return 0;
}

/* Thread creation */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
	int ret;

	while (__thread_fn != NULL)
		pthread_yield();

	__thread_fn = start_routine;
	ret = sys_thread_create(thread, attr, __thread_routine, arg);

	return ret;
}

/* Thread join / synchronisation */
int pthread_join(pthread_t thread, void **value_ptr) {

  return sys_thread_join(thread, value_ptr);
}

/* Modify thread priority */
int pthread_setschedprio(pthread_t thread, int prio) {
	return sys_sched_setparam(thread, prio);
}

/* Thread exit */
void pthread_exit(void *value_ptr) {
	sys_thread_exit(value_ptr);
}

void pthread_mutex_lock(void) {
	sys_mutex_lock(2);
}

void pthread_mutex_unlock(void) {
	sys_mutex_unlock(2);
}
