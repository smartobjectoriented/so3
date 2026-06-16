/*
 * Copyright (C) 2014-2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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

/* Copy a file descriptor to stdout. Returns 0 on success, -1 on error. */
static int cat_fd(int fd)
{
	char buf[1024];
	ssize_t n;

	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		if (write(STDOUT_FILENO, buf, n) != n)
			return -1;
	}

	return (n < 0) ? -1 : 0;
}

/* Concatenate files (or stdin when no argument is given) to stdout. */
int main(int argc, char **argv)
{
	int i, rc = 0;

	if (argc < 2)
		return cat_fd(STDIN_FILENO) < 0 ? 1 : 0;

	for (i = 1; i < argc; i++) {
		int fd = open(argv[i], O_RDONLY);

		if (fd < 0) {
			printf("cat: %s: cannot open\n", argv[i]);
			rc = 1;
			continue;
		}

		if (cat_fd(fd) < 0)
			rc = 1;

		close(fd);
	}

	return rc;
}
