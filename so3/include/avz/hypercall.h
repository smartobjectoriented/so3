/*
 * Copyright (C) 2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef HYPERCALL_H
#define HYPERCALL_H

#include <avz/domctl.h>

void do_domctl(domctl_t *args);

#endif /* HYPERCALL_H */
