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
#include <fcntl.h>

/* Copy the regular file SRC to DST (truncating/creating DST). */
int main(int argc, char **argv)
{
	char buf[1024];
	ssize_t n;
	int in, out, rc = 0;

	if (argc != 3) {
		printf("usage: cp SRC DST\n");
		return 1;
	}

	in = open(argv[1], O_RDONLY);
	if (in < 0) {
		printf("cp: cannot open '%s'\n", argv[1]);
		return 1;
	}

	out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (out < 0) {
		printf("cp: cannot create '%s'\n", argv[2]);
		close(in);
		return 1;
	}

	while ((n = read(in, buf, sizeof(buf))) > 0) {
		if (write(out, buf, n) != n) {
			printf("cp: write error\n");
			rc = 1;
			break;
		}
	}

	if (n < 0) {
		printf("cp: read error\n");
		rc = 1;
	}

	close(in);
	close(out);

	return rc;
}
