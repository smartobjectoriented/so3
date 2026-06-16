/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <stdio.h>
#include <unistd.h>

/* Remove one or more empty directories. */
int main(int argc, char **argv)
{
	int i, rc = 0;

	if (argc < 2) {
		printf("usage: rmdir DIR...\n");
		return 1;
	}

	for (i = 1; i < argc; i++) {
		if (rmdir(argv[i]) < 0) {
			printf("rmdir: failed to remove '%s'\n", argv[i]);
			rc = 1;
		}
	}

	return rc;
}
