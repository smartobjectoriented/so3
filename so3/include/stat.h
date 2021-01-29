/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Alexandre Malki <alexandre.malki@heig-vd.ch>
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

#ifndef STAT_H
#define STAT_H

#include <types.h>

#define FILENAME_SIZE 256

typedef uint32_t mode_t;

struct stat {
	char	st_name[FILENAME_SIZE];		/* Filename */
	unsigned long	st_size; 		/* Size of file */
	unsigned long	st_mtim;		/* Time of last modification in sec*/
	unsigned char	st_flags;		/* Regular file flag (not supported on fat) */
	mode_t	st_mode;			/* Protection not used (not supported on fat) */
};


#endif /* STAT_H */
