/*
 * Copyright (C) 2014-2019 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
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


#ifndef ASF_USR_H
#define ASF_USR_H

/* ASF char drv related constants */
#define ASF_DEV_MAJOR	100
#define ASF_DEV_NAME  	"/dev/soo/asf"

/* ASF IOCTL - Send cmd to Hello World TA */
#define ASF_IOCTL_CRYPTO_TEST		_IOW(0x5000DD30, 0, char)
#define ASF_IOCTL_HELLO_WORLD_TEST	_IOW(0x5000DD30, 1, char)
#define ASF_IOCTL_OPEN_SESSION		_IOW(0x5000DD30, 2, char)
#define ASF_IOCTL_SESSION_OPENED	_IOW(0x5000DD30, 3, char)
#define ASF_IOCTL_CLOSE_SESSION		_IOW(0x5000DD30, 4, char)

#endif /* ASF_USR_H */
