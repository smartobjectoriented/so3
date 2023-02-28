/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016-2018 Baptiste Delporte <bonel@bonel.net>
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

/*
 * Timestamp and delay measurement facility.
 *
 * There are two sets of functions: one for a non RT domain (non RT agency or ME), one for the
 * RT agency. The RT version is prefixed with rtdm_.
 *
 * Typical usage:
 *
 * 1. Record the timestamp at the beginning of the event of interest.
 *
 * ll_time_begin(index)
 *
 * where index is a slot number, from 0 to 7, allowing to make 7 independent measurements.
 *
 * 2. Record the timestamp at the end of the event of interest.
 *
 * ll_time_end(index)
 *
 * 3. Call the function that compute and bufferizes the delays (that is, the differences between
 * the beginning and end timestamps):
 *
 * ll_time_collect_delay_show(pre, index, post)
 *
 * where index is the slot number, pre is a prepended string and post is a postpended string.
 *
 * The function will compute 9 delays silently and show the bufferized results for the 10th delay.
 *
 */


#include <device/timer.h>

#include <soo/debug.h>
#include <soo/console.h>

#define N_DELAYS		4
#define N_DELAY_SAMPLES		10
#define N_TIMESTAMPS		10

static s64 delay_timestamp_begin[N_DELAYS] = { 0 };
static s64 delay_timestamp_end[N_DELAYS] = { 0 };
static s64 delay_samples[N_DELAYS][N_DELAY_SAMPLES];
static uint32_t delay_samples_count[N_DELAYS] = { 0 };
static s64 timestamps[N_DELAYS][N_DELAY_SAMPLES];
static uint32_t timestamps_count[N_DELAYS] = { 0 };
static s64 period_prev_timestamps[N_DELAYS][N_DELAY_SAMPLES];
static s64 periods[N_DELAYS][N_DELAY_SAMPLES];
static uint32_t periods_count[N_DELAYS] = { 0 };

s64 ll_time_get(void) {
	return get_s_time();
}

static s64 compute_delay(s64 t1, s64 t2) {
	return t2 - t1;
}

void ll_time_begin(uint32_t index) {
	delay_timestamp_begin[index] = ll_time_get();
}

void ll_time_end(uint32_t index) {
	delay_timestamp_end[index] = ll_time_get();
}

s64 get_time_delay(int index) {
	return compute_delay(delay_timestamp_begin[index], delay_timestamp_end[index]);
}

int collect_delay(uint32_t index, s64 *delay) {
	int ret = 0;

	delay_samples[index][delay_samples_count[index]] = get_time_delay(index);

	*delay = delay_samples[index][delay_samples_count[index]];

	if (unlikely(delay_samples_count[index] == N_DELAY_SAMPLES - 1))
		ret = 1;

	delay_samples_count[index] = (delay_samples_count[index] + 1) % N_DELAY_SAMPLES;

	return ret;
}

s64 ll_time_collect_delay(uint32_t index) {
	s64 current_delay;

	if (collect_delay(index, &current_delay))
		return current_delay;
	else
		return 0;
}

void ll_time_collect_delay_show(char *pre, uint32_t index, char *post) {
	uint32_t i;
	s64 current_delay = ll_time_collect_delay(index);

	if (current_delay) {
		lprintk(pre);
		for (i = 0; i < N_DELAY_SAMPLES; i++)
			lprintk("%u ", (uint32_t) (delay_samples[index][i]));
		lprintk(post);
		lprintk("\n");
	}
}

void ll_time_reset_delay_samples(uint32_t index) {
	uint32_t i;

	for (i = 0; i < N_DELAY_SAMPLES; i++)
		delay_samples[index][i] = 0;
	delay_samples_count[index] = 0;
}

int ll_time_collect_timestamp(uint32_t index) {
	int ret = 0;

	timestamps[index][timestamps_count[index]] = ll_time_get();

	if (unlikely(timestamps_count[index] == N_TIMESTAMPS - 1))
		ret = 1;

	timestamps_count[index] = (timestamps_count[index] + 1) % N_TIMESTAMPS;

	return ret;
}

void ll_time_collect_timestamp_show(char *pre, uint32_t index) {
	uint32_t i;

	if (ll_time_collect_timestamp(index)) {
		lprintk(pre);
		for (i = 0; i < N_TIMESTAMPS; i++)
			lprintk_int64_post(timestamps[index][i], " ");
		lprintk("\n");
	}
}

void ll_time_reset_timestamps(uint32_t index) {
	uint32_t i;

	for (i = 0; i < N_TIMESTAMPS; i++)
		timestamps[index][i] = 0;
	timestamps_count[index] = 0;
}

s64 get_time_period(int index, int count) {
	s64 timestamp = ll_time_get();
	s64 ret = compute_delay(period_prev_timestamps[index][count], timestamp);

	period_prev_timestamps[index][count] = timestamp;

	return ret;
}

int collect_period(uint32_t index, s64 *period) {
	int ret = 0;

	periods[index][periods_count[index]] = get_time_period(index, periods_count[index]);

	*period = periods[index][periods_count[index]];

	if (unlikely(periods_count[index] == N_DELAY_SAMPLES - 1))
		ret = 1;

	periods_count[index] = (periods_count[index] + 1) % N_DELAY_SAMPLES;

	return ret;
}

s64 ll_time_collect_period(uint32_t index) {
	s64 current_period;

	if (collect_period(index, &current_period))
		return current_period;
	else
		return 0;
}

void ll_time_collect_period_show(char *pre, uint32_t index, char *post) {
	uint32_t i;
	s64 current_period = ll_time_collect_period(index);

	if (current_period) {
		lprintk(pre);
		for (i = 0; i < N_DELAY_SAMPLES; i++)
			lprintk("%u ", (uint32_t) (periods[index][i]));
		lprintk(post);
		lprintk("\n");
	}
}

