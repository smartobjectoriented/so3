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

#ifndef BLIND_H
#define BLIND_H

#include <spinlock.h>
#include <printk.h>
#include <completion.h>
#include <asm/atomic.h>

#include <me/common.h>
#include <soo/knx/vbwa88pg.h>

#include <me/switch.h>

#define MEBLIND_NAME		"ME blind"
#define MEBLIND_PREFIX	"[ " MEBLIND_NAME " ] "

#if 1
#define BLIND_VBWA88PG
#endif

#define BLIND_MODEL "<model spid=\"00000200000000000000000000000001\">\
        <name>SOO.blind</name>\
        <description>\"SOO.blind permet de gérer la position des stores.\"</description>\
        <layout>\
            <row>\
                <col span=\"2\"><button id=\"blind-up\" lockable=\"true\" lockable-after=\"1.5\">STEP_UP</button></col>\
                <col span=\"2\"><button id=\"blind-down\" lockable=\"true\" lockable-after=\"1.5\">STEP_DOWN</button></col>\
            </row>\
            <row>\
                <col span=\"2\"><button id=\"blind-up-long\" lockable=\"true\" lockable-after=\"1.5\">UP</button></col>\
                <col span=\"2\"><button id=\"blind-down-long\" lockable=\"true\" lockable-after=\"1.5\">DOWN</button></col>\
            </row>\
        </layout>\
    </model>"


/**
 * @brief Blind models
 * 
 */
typedef enum {
	VBWA88PG = 0
} blind_type;

typedef enum {
	BLIND_UP = 0,
	BLIND_DOWN,
	BLIND_DIRECTION_NULL
} blind_direction_t;

typedef enum {
	BLIND_FULL = 0,
	BLIND_STEP,
	BLIND_ACTION_MODE_NULL
} blind_action_mode_t;

/**
 * @brief Generic blind struct. More kinds of blind can be added
 * 
 * @param blind specific blind model struct
 * @param type blind model
 * 
 */
typedef struct {
#ifdef BLIND_VBWA88PG
	blind_vbwa88pg_t blind;
#endif

	blind_type type;
} blind_t;


/*
 * Never use lock (completion, spinlock, etc.) in the shared page since
 * the use of ldrex/strex instructions will fail with cache disabled.
 */
/**
 * @brief Shared struct for blind ME
 * 
 * @param switch_event set to true if an switch event is received
 * @param cmd last switch command received
 * @param need_progate set to true if the ME need to migrate  
 */
typedef struct {
	bool blind_event;
	blind_direction_t direction;
	blind_action_mode_t action_mode;

    uint64_t originUID;
	uint64_t timestamp;
	bool need_propagate;
	bool delivered;
	/*
	 * MUST BE the last field, since it contains a field at the end which is used
	 * as "payload" for a concatened list of hosts.
	 */
	me_common_t me_common;

} sh_blind_t;

/* Export the reference to the shared content structure */
extern sh_blind_t *sh_blind;

/* Protecting variables between domcalls and the active context */
extern spinlock_t propagate_lock;

/**
 * @brief Completion use to wait for a switch event to move the blind
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

#endif /* BLIND_H */


