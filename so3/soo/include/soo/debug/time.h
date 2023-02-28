/*
 * Copyright (C) 2018 Baptiste Delporte <bonel@bonel.net>
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

#ifndef DEBUG_TIME_H
#define DEBUG_TIME_H

#include <soo/console.h>
#include <soo/debug.h>

s64 ll_time_get(void);
void ll_time_begin(uint32_t index);
void ll_time_end(uint32_t index);
s64 ll_time_collect_delay(uint32_t index);
void ll_time_collect_delay_show(char *pre, uint32_t index, char *post);
void ll_time_reset_delay_samples(uint32_t index);
void ll_time_collect_timestamp_show(char *pre, uint32_t index);
void ll_time_reset_timestamps(uint32_t index);
s64 ll_time_collect_period(uint32_t index);
void ll_time_collect_period_show(char *pre, uint32_t index, char *post);

#endif /* DEBUG_TIME_H */
