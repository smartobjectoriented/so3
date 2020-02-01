/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef bananapi_TIMER_H
#define bananapi_TIMER_H

#include <types.h>

#define TMR_IRQ_EN_REG		0x0
#define TMR_STATUS_REG		0x4
#define TMR_CTRL_REG 		0x10
#define TMR_INT_VALUE_REG	0X14
#define TMR_VALUE_REG 		0x18
#define TIMER_CTRL_IE 		0x1
#define TIMER_INTCLR		0x1
#define CONTINUOUS_MODE 	(0x1 << 6)
#define TIMER_CTRL_DIV16 	(0b100 << 5)
#define TIMER_CTRL_ENABLE 	0x1

#endif
