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

#include <stdio.h>
#include <sys/stat.h>

/* Create one or more directories. */
int main(int argc, char **argv)
{
	int i, rc = 0;

	if (argc < 2) {
		printf("usage: mkdir DIR...\n");
		return 1;
	}

	for (i = 1; i < argc; i++) {
		if (mkdir(argv[i], 0777) < 0) {
			printf("mkdir: cannot create directory '%s'\n", argv[i]);
			rc = 1;
		}
	}

	return rc;
}
