/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright (c) 1999-2002 Vojtech Pavlik
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef INPUT_H
#define INPUT_H

#include <device/timer.h>

#include "input-event-codes.h"

/*
 * The event structure itself
 */

struct input_event {
	struct timeval time;
	__u16 type;
	__u16 code;
	__s32 value;
};

#endif /* INPUT_H */



