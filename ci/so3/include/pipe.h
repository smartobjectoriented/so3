/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef PIPE_H
#define PIPE_H

#include <memory.h>
#include <mutex.h>
#include <completion.h>

#define PIPE_READER	0
#define PIPE_WRITER	0

#define PIPE_SIZE	PAGE_SIZE

struct pipe_desc {

	/* Mutex to access critical parts */
	struct mutex lock;

	/* Main buffer of this pipe */
	void *pipe_buf;

	/* The two global FD for each extremity. A value of -1 indicate an invalid gfd (finished for example) */
	int gfd[2];

	/* Consumer */
	int pos_read;

	/* Producer */
	uint32_t pos_write;

	/* Waiting queue for managing empty pipe */
	completion_t wait_for_writer;

	/* Waiting queue for managing full pipe */
  	completion_t wait_for_reader;
};
typedef struct pipe_desc pipe_desc_t;

int do_pipe(int pipefd[2]);

#endif /* PIPE_H */
