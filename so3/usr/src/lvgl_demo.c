/*
 * Copyright (C) 2025 André Costa <andre_miguel_costa@hotmail.com>
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

#include <demos/lv_demos.h>

#include <slv.h>

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage %s <lv_demo_name>\n", argv[0]);
		lv_demos_show_help();
		return EXIT_FAILURE;
	}
	slv_t slv;
	int err = slv_init(&slv);
	if (err < 0) {
		printf("Failed to init SLV\n");
		return EXIT_FAILURE;
	}
	lv_demos_create(argv + 1, argc - 1);

	slv_loop(&slv);

	slv_terminate(&slv);

	return EXIT_SUCCESS;
}
