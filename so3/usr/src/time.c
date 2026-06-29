/*
 * Copyright (C) 2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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

/* Continuously print the wall-clock time (seconds and microseconds from
 * gettimeofday(2)) in a tight loop, until the program is killed. */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <sys/time.h>
#include <inttypes.h>

int main(int argc, char *argv[])
{
	struct timeval tv;

	while (true) {
		gettimeofday(&tv, NULL);

		printf("# time(s) : %" PRIu64 "  time(us) : %" PRIu64 "\n", tv.tv_sec, tv.tv_usec);
	}
}
