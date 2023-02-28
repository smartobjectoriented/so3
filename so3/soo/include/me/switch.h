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

#ifndef _SWITCH_H_
#define _SWITCH_H_

#include <completion.h>
#include <spinlock.h>
#include <printk.h>
#include <completion.h>
#include <asm/atomic.h>

#include <me/common.h>
#include <soo/knx/gtl2tw.h>
#include <soo/enocean/ptm210.h>

#ifdef CONFIG_SOO_SWITCH_KNX
#define MESWITCH_NAME		"ME switch KNX"
#define KNX
#elif defined(CONFIG_SOO_SWITCH_ENOCEAN)
#define MESWITCH_NAME		"ME switch ENOCEAN"
#define ENOCEAN
#else
#define MESWITCH_NAME		"ME switch"
#endif

#define MESWITCH_PREFIX	    "[ " MESWITCH_NAME " ] "


/**
 * @brief Switch models
 * @param PTM210 Enocena switch
 * @param GTL2TW KNX switch
 */
typedef enum {
	PTM210 = 0,
	GTL2TW
} switch_type;

/**
 * @brief Possible switch press positions
 */
typedef enum {
	POS_LEFT_UP = 0,
	POS_LEFT_DOWN,
	POS_RIGHT_UP,
	POS_RIGHT_DOWN,
	POS_NONE
} switch_position;

/**
 * @brief  Possible press types
 */
typedef enum {
    PRESS_LONG = 0,
    PRESS_SHORT,
	PRESS_NONE
} switch_press;

/**
 * @brief  Possible switch status
 */
typedef enum {
	STATUS_OFF = 0,
	STATUS_ON,
	STATUS_NONE
} switch_status;

/**
 * @brief Generic switch struct
 * @param sw Specific switch model
 * @param type model
 */
typedef struct {
#ifdef ENOCEAN
	ptm210_t sw;
#endif

#ifdef KNX
	gtl2tw_t sw;
#endif
	switch_type type;

} switch_t;

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
	switch_position pos;
    switch_press press;
	switch_status status;
	switch_type type;
    uint64_t originUID;
    uint64_t timestamp;
	bool delivered;

	/** Private **/
	bool need_propagate;
	bool switch_event;

	/*
	 * MUST BE the last field, since it contains a field at the end which is used
	 * as "payload" for a concatened list of hosts.
	 */
	me_common_t me_common;

} sh_switch_t;

/* Export the reference to the shared content structure */
extern sh_switch_t *sh_switch;

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

#endif /* _SWITCH_H_ */


