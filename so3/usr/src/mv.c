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

/* Rename / move SRC to DST (same filesystem). */
int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("usage: mv SRC DST\n");
		return 1;
	}

	if (rename(argv[1], argv[2]) < 0) {
		printf("mv: cannot move '%s' to '%s'\n", argv[1], argv[2]);
		return 1;
	}

	return 0;
}
