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

#include <benchmark/lv_demo_benchmark.h>
#include <demos/lv_demos.h>

#include <slv.h>

#include <stdlib.h>
#include <stdio.h>

static slv_t slv;

void on_benchmark_end(const lv_demo_benchmark_summary_t *summary)
{
	lv_demo_benchmark_summary_display(summary);
	slv.terminate = true;
}

int main(int argc, char **argv)
{
	printf("Starting LVGL Benchmark\n");
	int err = slv_init(&slv);
	if (err < 0) {
		printf("Failed to init SLV\n");
		return EXIT_FAILURE;
	}
	lv_demo_benchmark_set_end_cb(on_benchmark_end);
	lv_demo_benchmark();

	slv_loop(&slv);

	slv_terminate(&slv);
	printf("LVGL Benchmark Over\n");

	return EXIT_SUCCESS;
}
