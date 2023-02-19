/*
 * Copyright (C) 2016 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2017,2018 Baptiste Delporte <bonel@bonel.net>
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

#ifndef DCM_H
#define DCM_H

#ifdef __KERNEL__
#include <asm-generic/ioctl.h>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __KERNEL__ */

#define DCM_DEV_NAME		"/dev/soo/dcm"

#ifdef __KERNEL__

#define DCM_MAJOR		127

#define DCM_N_RECV_BUFFERS	10

/* Max ME size in bytes */
#define DATACOMM_ME_MAX_SIZE	(32 * 1024 * 1024)

#endif /* __KERNEL__ */

/* IOCTL codes exposed to the user space side */
#define DCM_IOCTL_NEIGHBOUR_COUNT		_IOWR(0x5000DC30, 0, char)
#define DCM_IOCTL_SEND				_IOWR(0x5000DC30, 1, char)
#define DCM_IOCTL_RECV				_IOWR(0x5000DC30, 2, char)
#define DCM_IOCTL_RELEASE			_IOWR(0x5000DC30, 3, char)
#define DCM_IOCTL_DUMP_NEIGHBOURHOOD		_IOWR(0x5000DC30, 4, char)
#define DCM_IOCTL_SET_AGENCY_UID		_IOWR(0x5000DC30, 5, char)

typedef struct {
	void *ME_data;	/* Reference to the uncompressed ME */
	size_t size;		/* Size of this ME ready to be compressed */
	uint32_t prio;		/* Priority of this ME (unused at the moment) */
} dcm_ioctl_send_args_t;

typedef struct {
	void *ME_data;
	size_t buffer_size;
	size_t ME_size;
} dcm_ioctl_recv_args_t;

/*
 * Types of buffer; helpful to manage buffers in a seamless way.
 */
typedef enum {
	DCM_BUFFER_SEND = 0,
	DCM_BUFFER_RECV
} dcm_buffer_direction_t;

#ifdef __KERNEL__

/*
 * - DCM_BUFFER_FREE means the buffer is ready for send/receive operations.
 * - DCM_BUFFER_BUSY means the buffer is reserved for a ME.
 * - DCM_BUFFER_SENDING means the buffer is currently along the path for being sent out.
 *   The buffer can still be altered depending on the steps of sending (See SOOlink/Coder).
 */
typedef enum {
	DCM_BUFFER_FREE = 0,
	DCM_BUFFER_BUSY,
	DCM_BUFFER_SENDING,
} dcm_buffer_status_t;

/*
 * Buffer descriptor
 */
typedef struct {
	dcm_buffer_status_t	status;

	/*
	 * Reference to the ME buffer.
	 */
	void *ME_data;
	size_t size;

	uint32_t prio;

} dcm_buffer_desc_t;

#endif /* __KERNEL__ */

#ifdef __KERNEL__
int dcm_ME_rx(void *ME_buffer, size_t size);
#endif /* __KERNEL__ */

#endif /* DCM_H */
