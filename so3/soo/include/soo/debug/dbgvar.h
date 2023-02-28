/*
 * Copyright (C) 2018 Baptiste Delporte <bonel@bonel.net>
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

#ifndef DBGVAR_H
#define DBGVAR_H

#define DBGEVENT_SIZE 	8192
#define DBGLIST_SIZE 	8192

typedef enum {
	DBGVAR_NR
} dbgvar_t;

extern int dbgvar[DBGVAR_NR];

/*
 * List mode:
 * - DBGLIST_DISABLED: the list is not used, thus inactive.
 * - DBGLIST_CLEAR: the list is cleared when it is full, that is, when there are DBG*_SIZE.
 * - DBGLIST_CONTINUOUS: the list is a circular buffer.
 */
typedef enum {
	DBGLIST_DISABLED,
	DBGLIST_CLEAR,
	DBGLIST_CONTINUOUS
} dbglist_mode_t;

/*
 * Clear the event list.
 */
void clear_dbgevent(void);

/*
 * Log an event.
 * Each event is associated to an arbitrary one-char key.
 */
void log_dbgevent(char c);

/*
 * Change the event logging mode.
 */
void set_dbgevent_mode(dbglist_mode_t mode);

/*
 * Show the event list.
 */
void show_dbgevent(void);

/*
 * Initialize the event list.
 */
void init_dbgevent(void);

/*
 * Clear the event list.
 */
void clear_dbglist(void);

/*
 * Log an event.
 * Each element is a 32 bit integer (signed or unsigned).
 */
void log_dbglist(int i);

/*
 * Change the integer logging mode.
 */
void set_dbglist_mode(dbglist_mode_t mode);

/*
 * Show the event list. The elements are printed in hexadecimal format.
 */
void show_dbglist(void);

/*
 * Clear the integer list.
 */
void init_dbglist(void);

#endif /* DBGVAR_H */
