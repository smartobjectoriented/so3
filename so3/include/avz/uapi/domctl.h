/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef __DOMCTL_H__
#define __DOMCTL_H__

#include <avz/uapi/avz.h>

/*
 * There are two main scheduling policies: the one used for normal (standard) ME, and
 * a second one used for realtime ME.
 */
#define AVZ_SCHEDULER_FLIP	0
#define AVZ_SCHEDULER_RT	1

#define DOMCTL_pauseME       	1
#define DOMCTL_unpauseME     	2
#define DOMCTL_get_AVZ_shared	3

struct domctl_unpause_ME {
	uint32_t vbstore_pfn;
};

struct domctl {
    uint32_t cmd;
    domid_t  domain;
    union {
       struct domctl_unpause_ME	unpause_ME;
       addr_t avz_shared_paddr;
    } u;
};
typedef struct domctl domctl_t;

#endif /* __DOMCTL_H__ */


