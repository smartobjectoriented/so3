/*
 * Copyright (C) 2016-2019 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016-2019 Baptiste Delporte <bonel@bonel.net>
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

/*
 * Debugging interface that provides event logging and integer logging features. Two lists are provided:
 * - a list of events, composed of a unique one-character desscriptor
 * - a list of integers, composed of 32 bit integers
 * Each new element is added to the dedicated list. The list can be printed afterwards.
 *
 * This variable list management facility may be used for debugging purposes. There are two possible ways to monitor variables and to display their content via sysfs:
 * 1) Static variable which can be added as an enum part of dbgvar_t typedef. The variable is stored in the dbgvar[] array.
 * 2) Dynamic logging using the *_dbgevent() function.
 */


#include <string.h>
#include <heap.h>
#include <compiler.h>

#include <soo/console.h>
#include <soo/debug/dbgvar.h>

/* The following debug variable is used to monitor entry/exit in various functions called in a path. */
int dbgvar[DBGVAR_NR];

static dbglist_mode_t dbgevent_mode = DBGLIST_DISABLED;
static char *dbgevent;
static char *dbgevent_print;
static unsigned int dbgevent_count = 0;

static dbglist_mode_t dbglist_mode = DBGLIST_DISABLED;
static int *dbglist;
static int *dbglist_print;
static unsigned int dbglist_count = 0;

void clear_dbgevent(void) {
	if (dbgevent_mode != DBGLIST_DISABLED)
		memset(dbgevent, 0, DBGEVENT_SIZE);
}

void log_dbgevent(char c) {
	if (dbgevent_mode == DBGLIST_DISABLED)
		return;

	dbgevent[dbgevent_count] = c;

	if (unlikely(dbgevent_count >= DBGEVENT_SIZE))
		dbgevent_count = 0;
	else
		dbgevent_count++;
	if (unlikely((dbgevent_mode == DBGLIST_CLEAR) && (dbgevent_count == 0)))
		clear_dbgevent();
}

void set_dbgevent_mode(dbglist_mode_t mode) {
	dbgevent_mode = mode;
}

void show_dbgevent(void) {
	int i;
	unsigned int cur_dbgevent_count = dbgevent_count;
	memcpy(dbgevent_print, dbgevent, DBGEVENT_SIZE);

	if (dbgevent_mode == DBGLIST_CLEAR) {
		for (i = 0 ; i < DBGEVENT_SIZE ; i++) {
			if (dbgevent_print[i] != 0)
				lprintk("%c", dbgevent_print[i]);
		}
	}
	else if (dbgevent_mode == DBGLIST_CONTINUOUS) {
		i = (cur_dbgevent_count + 1) % DBGEVENT_SIZE;
		while (i != cur_dbgevent_count) {
			if (dbgevent_print[i] != 0)
				lprintk("%c", dbgevent_print[i]);
			i = (i + 1) % DBGEVENT_SIZE;
		}
	}
	lprintk("\n");
}

void init_dbgevent(void) {
	/* Initialize the debugging list */
	dbgevent = (char *) malloc(DBGEVENT_SIZE);
	dbgevent_print = (char *) malloc(DBGEVENT_SIZE);
}

void delete_dbgevent(void) {
	free(dbgevent);
	free(dbgevent_print);
}

void clear_dbglist(void) {
	if (dbglist_mode != DBGLIST_DISABLED)
		memset(dbglist, 0, DBGLIST_SIZE * sizeof(int));
}

void log_dbglist(int i) {
	if (dbglist_mode == DBGLIST_DISABLED)
		return;

	dbglist[dbglist_count] = i;

	if (unlikely(dbglist_count >= DBGLIST_SIZE))
		dbglist_count = 0;
	else
		dbglist_count++;
	if (unlikely((dbglist_mode == DBGLIST_CLEAR) && (dbglist_count == 0)))
		clear_dbglist();
}

void set_dbglist_mode(dbglist_mode_t mode) {
	dbglist_mode = mode;
}

void show_dbglist(void) {
	int i;
	unsigned int cur_dbglist_count = dbglist_count;
	memcpy(dbglist_print, dbglist, DBGLIST_SIZE * sizeof(int));

	if (dbglist_mode == DBGLIST_CLEAR) {
		for (i = 0 ; i < DBGLIST_SIZE ; i++) {
			if (dbglist_print[i] != 0)
				lprintk("%08x\n", dbglist_print[i]);
		}
	}
	else if (dbglist_mode == DBGLIST_CONTINUOUS) {
		i = (cur_dbglist_count + 1) % DBGLIST_SIZE;
		while (i != cur_dbglist_count) {
			if (dbglist_print[i] != 0)
				lprintk("%08x\n", dbglist_print[i]);
			i = (i + 1) % DBGLIST_SIZE;
		}
	}
}

void init_dbglist(void) {
	/* Initialize the debugging list */
	dbglist = (int *) malloc(DBGLIST_SIZE * sizeof(int));
	dbglist_print = (int *) malloc(DBGLIST_SIZE * sizeof(int));
}

void delete_dbglist(void) {
	free(dbglist);
	free(dbglist_print);
}
