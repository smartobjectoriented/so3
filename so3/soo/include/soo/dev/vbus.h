/*
 * Copyright (C) 2016-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef _IO_VBUS_H
#define _IO_VBUS_H

/*
 * The following states are used to synchronize backend and frontend activities.
 * Detailed description is given in the SOO Framework technical reference.
 */

enum vbus_state
{
	VbusStateUnknown      = 0,
	VbusStateInitialising = 1,
	VbusStateInitWait     = 2,
	VbusStateInitialised  = 3,
	VbusStateConnected    = 4,
	VbusStateClosing      = 5,
	VbusStateClosed       = 6,
	VbusStateReconfiguring = 7,
	VbusStateReconfigured  = 8,
	VbusStateSuspending    = 9,
	VbusStateSuspended     = 10,
	VbusStateResuming      = 11

};

#endif /* _IO_VBUS_H */
