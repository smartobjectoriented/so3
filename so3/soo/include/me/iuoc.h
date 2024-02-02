/*
 * Copyright (C) 2023 A.Gabriel Catel Torres <arzur.cateltorres@heig-vd.ch>
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
 * Description: This file is the implementation of the IUOC ME. This code is 
 * responsible of managing the data incoming from any ME and from the IUOC server
 * that are allowed to communicate with the IUOC.
 */

#ifndef IUOC_H
#define IUOC_H

#include <completion.h>
#include <spinlock.h>
#include <printk.h>
#include <asm/atomic.h>

#include <me/common.h>
#include <me/iuoc.h>
#include <me/blind.h>

#define MEIUOC_NAME		"ME iuoc"
#define MEIUOC_PREFIX	"[ " MEIUOC_NAME " ]"


/*
 * Never use lock (completion, spinlock, etc.) in the shared page since
 * the use of ldrex/strex instructions will fail with cache disabled.
 */
typedef struct {
	int received_data;
	sh_blind_t *sh_blind;
	
	/*
	 * MUST BE the last field, since it contains a field at the end which is used
	 * as "payload" for a concatened list of hosts.
	 */
	me_common_t me_common;

} sh_iuoc_t;

/* Export the reference to the shared content structure */
extern sh_iuoc_t *sh_iuoc;

/**
 * @brief Condition of which the threads are running
 * 
 */
extern atomic_t shutdown;

#define pr_err(fmt, ...) \
	do { \
		printk("[%s:%i] Error: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#endif /* IUOC_H */


