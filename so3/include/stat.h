/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Alexandre Malki <alexandre.malki@heig-vd.ch>
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

#ifndef STAT_H
#define STAT_H

struct stat {
	unsigned long st_dev; /* Device.  */
	unsigned long st_ino; /* File serial number.  */
	unsigned int st_mode; /* File mode.  */
	unsigned int st_nlink; /* Link count.  */
	unsigned int st_uid; /* User ID of the file's owner.  */
	unsigned int st_gid; /* Group ID of the file's group. */
	unsigned long st_rdev; /* Device number, if device.  */
	unsigned long __pad1;
	long st_size; /* Size of file, in bytes.  */
	int st_blksize; /* Optimal block size for I/O.  */
	int __pad2;
	long st_blocks; /* Number 512-byte blocks allocated. */
	long st_atime; /* Time of last access.  */
	unsigned long st_atime_nsec;
	long st_mtime; /* Time of last modification.  */
	unsigned long st_mtime_nsec;
	long st_ctime; /* Time of last status change.  */
	unsigned long st_ctime_nsec;
	unsigned int __unused4;
	unsigned int __unused5;
};

struct stat64 {
	unsigned long long st_dev; /* Device.  */
	unsigned long long st_ino; /* File serial number.  */
	unsigned int st_mode; /* File mode.  */
	unsigned int st_nlink; /* Link count.  */
	unsigned int st_uid; /* User ID of the file's owner.  */
	unsigned int st_gid; /* Group ID of the file's group. */
	unsigned long long st_rdev; /* Device number, if device.  */
	unsigned long long __pad1;
	long long st_size; /* Size of file, in bytes.  */
	int st_blksize; /* Optimal block size for I/O.  */
	int __pad2;
	long long st_blocks; /* Number 512-byte blocks allocated. */
	int st_atime; /* Time of last access.  */
	unsigned int st_atime_nsec;
	int st_mtime; /* Time of last modification.  */
	unsigned int st_mtime_nsec;
	int st_ctime; /* Time of last status change.  */
	unsigned int st_ctime_nsec;
	unsigned int __unused4;
	unsigned int __unused5;
};

#endif /* STAT_H */
