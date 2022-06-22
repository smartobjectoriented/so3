/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2017 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
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

#ifndef VFS_H
#define VFS_H

/* File access mode flags */
#define O_SEARCH  O_PATH
#define O_EXEC    O_PATH

#define O_ACCMODE (03|O_SEARCH)
#define O_RDONLY  00
#define O_WRONLY  01
#define O_RDWR    02

/* Operating mode flags */

#define O_CREAT        0100
#define O_EXCL         0200
#define O_NOCTTY       0400
#define O_TRUNC       01000
#define O_APPEND      02000
#define O_NONBLOCK    04000
#define O_DSYNC      010000
#define O_SYNC     04010000
#define O_RSYNC    04010000
#define O_DIRECTORY  040000
#define O_NOFOLLOW  0100000
#define O_CLOEXEC  02000000

#define O_ASYNC      020000
#define O_DIRECT    0200000
#define O_LARGEFILE 0400000
#define O_NOATIME  01000000
#define O_PATH    010000000
#define O_TMPFILE 020040000
#define O_NDELAY O_NONBLOCK

#define MAX_FS_REGISTERED          4
#define MAX_DEV_CLASS_REGISTERED   4
#define MAX_FDS                  128

#define FILENAME_MAX 100

#define STDIN 0
#define STDOUT 1
#define STDERR 2

/* Types of lseek whence */
#define SEEK_SET        0       /* seek relative to beginning of file */
#define SEEK_CUR        1       /* seek relative to current file position */
#define SEEK_END        2       /* seek relative to end of file */
#define SEEK_DATA       3       /* seek to the next data */
#define SEEK_HOLE       4       /* seek to the next hole */
#define SEEK_MAX        SEEK_HOLE

/* Types of entry */
#define VFS_TYPE_FILE		0
#define VFS_TYPE_DIR		1

#define VFS_TYPE_PIPE		2
#define VFS_TYPE_IO		3	/* stdin/stdout/stderr */

#define VFS_TYPE_DEV_FB		4	/* framebuffer device */
#define VFS_TYPE_DEV_INPUT	5	/* input device (e.g. mouse) */
#define VFS_TYPE_DEV_CHAR	6       /* Generic character device */
#define VFS_TYPE_DEV_SOCK	7   	/* Sockets */
#define VFS_TYPE_DEV_NIC	8   	/* Network Interface Cards (NIC) */

/* Device type (borrowed from Linux) */
#define DT_UNKNOWN	0
#define DT_FIFO		1	/* Like pipe */
#define DT_CHR		2	/* Char device */
#define DT_DIR		4	/* DIR entry */
#define DT_BLK		6	/* Block device */
#define DT_REG		8	/* Regular file */
#define DT_LNK		10	/* Symbolic link */
#define DT_SOCK		12	/* Socket device */

#ifndef __ASSEMBLY__

#include <types.h>
#include <stat.h>
#include <dirent.h>

#include <device/device.h>

struct file_operations {
	int (*open)(int fd, const char *path);
	int (*close)(int fd);
	int (*read)(int fd, void *buffer, int count);
	int (*write)(int fd, const void *buffer, int count);
	off_t (*lseek)(int fd, off_t off, int whence);
	int (*ioctl)(int fd, unsigned long cmd, unsigned long args);
	struct dirent *(*readdir)(int fd);
	int (*mkdir)(int fd, void *);
	int (*stat)(const char *path, struct stat *st);
	void* (*mmap)(int fd, addr_t virt_addr, uint32_t page_count);
	int (*unlink)(int fd, void *);
	int (*mount)(const char *);
	int (*unmount)(const char *);
	void (*clone)(int fd);
};

struct fd {

	/* Filename */
	char *filename;

	/*  FD number */
	uint32_t val;
	uint32_t flags_operating_mode;
	uint32_t flags_access_mode;
	uint32_t flags_open;
	uint32_t type;
	 
	/* Reference counter to keep the object alive when greater than 0. */
	uint32_t ref_count;

	/* List of callbacks */
	struct file_operations *fops;

	/* Private data of fd */
	void *priv;
};

/* Syscall accessible from userspace */

int do_open(const char *filename, int flags);
int do_read(int fd, void *buffer, int count);
int do_write(int fd, const void *buffer, int count);
int do_readdir(int fd, char *buf, int len);
void do_close(int fd);
int do_dup(int oldfd);
int do_dup2(int oldfd, int newfd);
int do_stat(const char *path , struct stat *st);
void *do_mmap(addr_t start, size_t length, int prot, int fd, off_t offset);
int do_ioctl(int fd, unsigned long cmd, unsigned long args);
int do_fcntl(int fd, unsigned long cmd, unsigned long args);
off_t do_lseek(int fd, off_t off, int whence);

/* VFS common interface */

char *vfs_get_filename(int gfd);
int vfs_get_gfd(int localfd);
struct file_operations *vfs_get_fops(uint32_t gfd);
int vfs_refcount(int gfd);
void vfs_init(void);
int vfs_open(const char *filename, struct file_operations *fops, uint32_t type);
int vfs_close(int gfd);
void vfs_set_priv(int gfd, void *data);
void *vfs_get_priv(int gfd);
int vfs_clone_fd(int *gfd_src, int *gfd_dst);

uint32_t vfs_get_access_mode(int fd);
uint32_t vfs_get_open_mode(int fd);
uint32_t vfs_get_operating_mode(int fd);

void vfs_set_access_mode(int gfd, uint32_t flags_access_mode);
int vfs_set_open_mode(int gfd, uint32_t flags_open_mode);
int vfs_set_operating_mode(int gfd, uint32_t flags_operating_mode);

#endif /* __ASSEMBLY__ */

#endif /* VFS_H */
