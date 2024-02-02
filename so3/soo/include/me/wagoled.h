/*
 * Copyright (C) 2022 Mattia Gallacchi <mattia.gallaccchi@heig-vd.ch>
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

#ifndef WAGOLED_H
#define WAGOLED_H

#include <completion.h>
#include <spinlock.h>
#include <printk.h>
#include <asm/atomic.h>

#include <me/common.h>
#include <me/switch.h>


#define MEWAGOLED_NAME		"ME wagoled"
#define MEWAGOLED_PREFIX	"[ " MEWAGOLED_NAME " ]"


#define WAGOLED_MODEL  "<model spid=\"00000200000000000000000000000003\">\
		<name>SOO.wagoled</name>\
		<description>\"SOO.blind permet de gérer l'allumage d'un luminaire Wago\"</description>\
		<layout>\
			<row>\
				<col span=\"2\"><label for=\"btn-led-l\">Lamp gauche :</label></col>\
				<col span=\"2\"><button id=\"btn-led-l\" lockable=\"true\" lockable-after=\"1.5\">Click</button></col>\
				<col span=\"2\"><label for=\"btn-led-r\">Lamp droite :</label></col>\
				<col span=\"2\"><button id=\"btn-led-r\" lockable=\"true\" lockable-after=\"1.5\">Click</button></col>\
			</row>\
		</layout>\
	</model>"


/*
 * Never use lock (completion, spinlock, etc.) in the shared page since
 * the use of ldrex/strex instructions will fail with cache disabled.
 */
typedef struct {

	bool switch_event;
	switch_position sw_pos;
	switch_status sw_status;
	/*
	 * MUST BE the last field, since it contains a field at the end which is used
	 * as "payload" for a concatened list of hosts.
	 */
	me_common_t me_common;

} sh_wagoled_t;

/* Export the reference to the shared content structure */
extern sh_wagoled_t *sh_wagoled;

/**
 * @brief Completion used to wait for a switch event
 * 
 */
extern struct completion send_data_lock;

/**
 * @brief Condition of which the threads are running
 * 
 */
extern atomic_t shutdown;

#define pr_err(fmt, ...) \
	do { \
		printk("[%s:%i] Error: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#endif /* WAGOLED_H */


