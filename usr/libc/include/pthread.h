/*
 * Copyright (C) 2014-2017 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef PTHREAD_H_
#define PTHREAD_H_

#include <bits/alltypes.h>


typedef int pthread_t;

/* Thread creation */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);

/* Thread join / synchronisation */
int pthread_join(pthread_t thread, void **value_ptr);

/* Modify thread priority */
int pthread_setschedprio(pthread_t thread, int prio);

/* Thread exit */
void pthread_exit(void *value_ptr);

/* Thread yield */
int pthread_yield(void);

#ifdef TEST_LABO03
void pthread_test_start(void);
int pthread_test_verif(void);
#endif

#endif /* PTHREAD_H_ */
