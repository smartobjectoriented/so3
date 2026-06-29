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

/* echo: write the command-line arguments to stdout, separated by spaces and
 * followed by a newline. */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++)
		printf("%s ", argv[i]);

	printf("\n");

	return 0;
}
