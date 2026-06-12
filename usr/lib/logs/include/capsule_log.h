/*
 * Copyright (C) 2024 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
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

#ifndef CAPSULE_LOG_H
#define CAPSULE_LOG_H

/*
 * Generates logs messages. It does:
 *   1. Add '[S3C:<S3C_ID>]' prefix to the message
 *   2. Send message though vLOGS
 *   3. The message is added in a file (`/var/log/soo/me_<SLOT-ID>.log) on the
 *      agency (SOO environment)
 *
 *   Warnings: It works only on Capsule - SO3 running on SOO environment
 */
void capsule_log(const char *fmt, ...);

#endif /* CAPSULE_LOG_H */