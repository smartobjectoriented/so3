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
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/*
 * Create each file if it does not exist. (SO3 has no utimensat, so an existing
 * file is simply left untouched rather than having its timestamp refreshed.)
 */
int main(int argc, char **argv)
{
	int i, rc = 0;

	if (argc < 2) {
		printf("usage: touch FILE...\n");
		return 1;
	}

	for (i = 1; i < argc; i++) {
		int fd = open(argv[i], O_WRONLY | O_CREAT, 0644);
		struct stat st;

		if (fd >= 0) {
			close(fd);
			continue;
		}

		/* open(O_CREAT) fails if the file already exists — that is fine. */
		if (stat(argv[i], &st) == 0)
			continue;

		printf("touch: cannot create '%s'\n", argv[i]);
		rc = 1;
	}

	return rc;
}
