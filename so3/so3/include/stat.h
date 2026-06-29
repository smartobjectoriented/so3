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

#include <types.h>

/* File type bits of st_mode (subset of the POSIX/Linux values). */
#define S_IFMT 0170000 /* type mask */
#define S_IFDIR 0040000 /* directory */
#define S_IFREG 0100000 /* regular file */

#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)

/* Stat structure copied from Linux include/uapi/asm-generic/stat.h */
struct stat {
	unsigned long st_dev; /* Device. */
	unsigned long st_ino; /* File serial number. */
	unsigned int st_mode; /* File mode. */
	unsigned int st_nlink; /* Link count. */
	unsigned int st_uid; /* User ID of the file's owner. */
	unsigned int st_gid; /* Group ID of the file's group. */
	unsigned long st_rdev; /* Device number, if device. */
	unsigned long __pad1;
	long st_size; /* Size of file, in bytes. */
	int st_blksize; /* Optimal block size for I/O. */
	int __pad2;
	long st_blocks; /* Number 512-byte blocks allocated. */
	long st_atime; /* Time of last access. */
	unsigned long st_atime_nsec;
	long st_mtime; /* Time of last modification. */
	unsigned long st_mtime_nsec;
	long st_ctime; /* Time of last status change. */
	unsigned long st_ctime_nsec;
	unsigned int __unused4;
	unsigned int __unused5;
};

/* This is for ARM32 compatibility as 64bits version of stat will be called.
   Adapted from arch/arm64/include/asm/stat.h on Linux */
struct stat64 {
	u64 st_dev; /* Device. */
	unsigned char __pad0[4];
	u32 __st_ino; /* File serial number. */
	u32 st_mode; /* File mode. */
	u32 st_nlink; /* Link count. */
	u32 st_uid; /* User ID of the file's owner. */
	u32 st_gid; /* Group ID of the file's group. */
	u64 st_rdev; /* Device number, if device. */
	unsigned char __pad3[4];
	s64 st_size; /* Size of file, in bytes. */
	u32 st_blksize; /* Optimal block size for I/O. */
	u64 st_blocks; /* Number 512-byte blocks allocated. */
	u32 st_atime; /* Time of last access. */
	u32 st_atime_nsec;
	u32 st_mtime; /* Time of last modification. */
	u32 st_mtime_nsec;
	u32 st_ctime; /* Time of last status change. */
	u32 st_ctime_nsec;
	u64 st_ino;
};

#endif /* STAT_H */
