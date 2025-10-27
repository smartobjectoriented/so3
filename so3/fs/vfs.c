/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2016-2017 Alexandre Malki <alexandre.malki@heig-vd.ch>
 * Copyright (C) 2016-2017 Xavier Ruppen <xavier.ruppen@heig-vd.ch>
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

#include <common.h>
#include <vfs.h>
#include <process.h>
#include <heap.h>
#include <errno.h>
#include <process.h>
#include <mutex.h>
#include <string.h>
#include <dirent.h>
#include <console.h>
#include <log.h>

#include <fat/fat.h>
#include <devfs/devfs.h>

#include <device/device.h>
#include <asm/mmu.h>

/* The VFS abstract subsystem manages a table of open file descriptors where indexes are known as gfd (global file descriptor).
 * Every process has its own file descriptor table. A local file descriptor (belonging to a process) must be linked to a global file descriptor
 * according to the fd type.
 */

struct mutex vfs_lock;

/* Available file descriptors. An entry is NULL when free */
struct fd *open_fds[MAX_FDS];

/* Registered file system operations - This is specific to a file system type. Currently, only FAT and pipe is used. */
/* Pipe has its own fops which is not put in this table. */
struct file_operations *registered_fs_ops[MAX_FS_REGISTERED];

/*
 * Check the validity of a gfd
 */
static bool vfs_is_valid_gfd(int gfd)
{
	ASSERT(mutex_is_locked(&vfs_lock));

	if ((gfd < 0) || !open_fds[gfd])
		return false;

	return true;
}

/* @brief This function will retrieve the index in the
 *		opend_fds from a process file descriptor
 * @param localfd: The process file descriptor
 * @return The corresponding fd upon success,
 *		-1 otherwise.
 */
int vfs_get_gfd(int localfd)
{
	pcb_t *pcb = current()->pcb;
	int gfd;

	/* Basic validation of fd */
	if (localfd < 0)
		return -1;

	mutex_lock(&vfs_lock);

	gfd = pcb->fd_array[localfd];

	if (!open_fds[gfd]) {
		mutex_unlock(&vfs_lock);
		return -1;
	}

	mutex_unlock(&vfs_lock);

	return gfd;
}

/*
 * @brief Get the refcount of a specific global fd.
 *
 */
int vfs_refcount(int gfd)
{
	int ret;

	mutex_lock(&vfs_lock);

	if (!vfs_is_valid_gfd(gfd)) /* May already disappear */ {
		mutex_unlock(&vfs_lock);
		return 0;
	}

	ret = open_fds[gfd]->ref_count;

	mutex_unlock(&vfs_lock);

	return ret;
}

/**
 * This function will increment the global fd ref_count to keep track on how many instances
 * are using this global descriptor.
 *
 * @param gfd
 * @return
 */
static int vfs_inc_ref(int gfd)
{
	int ret;

	mutex_lock(&vfs_lock);

	if (!vfs_is_valid_gfd(gfd)) {
		mutex_unlock(&vfs_lock);
		return -1;
	}

	open_fds[gfd]->ref_count++;

	ret = open_fds[gfd]->ref_count;

	mutex_unlock(&vfs_lock);

	return ret;
}

/* @brief This function allows to set "private data"
 * @param gfd
 * @param fd: This is the index on the open_fds
 * @param data: This is a pointer onto the "private data" to reuse later
 */
void vfs_set_priv(int gfd, void *data)
{
	open_fds[gfd]->priv = data;
}

/* @brief This function allows to retrieve "private data"
 * @param fgd
 * @param fd: This is the index on the open_fds
 * @return data: This is a pointer onto the "private data" to retrieve. It must be casted.
 */
void *vfs_get_priv(int gfd)
{
	return open_fds[gfd]->priv;
}

/*
 * Get type of file descriptor (VFS TYPE)
 */
int vfs_get_type(int gfd)
{
	return open_fds[gfd]->type;
}

/*
 * Get the filename associated to a file descriptor
 */
char *vfs_get_filename(int gfd)
{
	return open_fds[gfd]->filename;
}

/*
 * Get the reference to the fops associated to a gfd.
 */
struct file_operations *vfs_get_fops(uint32_t gfd)
{
	ASSERT(mutex_is_locked(&vfs_lock));

	if (!vfs_is_valid_gfd(gfd))
		return NULL;

	return open_fds[gfd]->fops;
}

/*
 * @brief This function opens a global file descriptors
 *		and return a process file descriptor
 *		linked to the global file descriptor.
 *
 * @param fd: This is the index on the open_fds
 * @return a new (local) fd for the running process
 */
int vfs_open(const char *filename, struct file_operations *fops, uint32_t type)
{
	int gfd, fd;

	mutex_lock(&vfs_lock);

	/* Find the first available file descriptor */
	for (gfd = 0; gfd < MAX_FDS; gfd++) {
		if (!open_fds[gfd])
			break;
	}

	/* Reach max num */
	if (gfd == MAX_FDS) {
		fd = -ENFILE;
		goto vfs_open_failed;
	}

	open_fds[gfd] = (struct fd *) malloc(sizeof(struct fd));

	if (!open_fds[gfd]) {
		printk("%s: failed to allocate memory\n", __func__);
		fd = -ENOMEM;
		goto vfs_open_failed;
	}

	memset(open_fds[gfd], 0, sizeof(struct fd));

	/* Store the filename */
	if (filename) {
		open_fds[gfd]->filename = malloc(strlen(filename) + 1);
		if (!open_fds[gfd]) {
			printk("%s: failed to allocate memory\n", __func__);
			fd = -ENOMEM;
			goto vfs_open_failed;
		}

		strcpy(open_fds[gfd]->filename, filename);
	}

	open_fds[gfd]->fops = fops;
	open_fds[gfd]->type = type;

	/* Increment open fd reference counter */
	vfs_inc_ref(gfd);

#ifdef CONFIG_PROC_ENV
	fd = proc_register_fd(gfd);

	if (fd < 0)
		goto fs_open_failed;
#else
	fd = gfd;
#endif

	mutex_unlock(&vfs_lock);

	return fd;

#ifdef CONFIG_PROC_ENV
fs_open_failed:
#endif

	free(open_fds[gfd]);
	open_fds[gfd] = NULL;

vfs_open_failed:

	mutex_unlock(&vfs_lock);
	return fd;
}

uint32_t vfs_get_open_mode(int gfd)
{
	ASSERT(mutex_is_locked(&vfs_lock));

	if (open_fds[gfd])
		return open_fds[gfd]->flags_open;

	return 0;
}

int vfs_set_open_mode(int gfd, uint32_t flags_open_mode)
{
	mutex_lock(&vfs_lock);
	open_fds[gfd]->flags_open = flags_open_mode;
	mutex_unlock(&vfs_lock);

	return 0;
}

uint32_t vfs_get_access_mode(int gfd)
{
	uint32_t ret = 0;

	mutex_lock(&vfs_lock);

	if (open_fds[gfd])
		ret = open_fds[gfd]->flags_access_mode;

	mutex_unlock(&vfs_lock);

	return ret;
}

void vfs_set_access_mode(int gfd, uint32_t flags_access_mode)
{
	mutex_lock(&vfs_lock);

	open_fds[gfd]->flags_access_mode = flags_access_mode;

	mutex_unlock(&vfs_lock);
}

uint32_t vfs_get_operating_mode(int gfd)
{
	uint32_t ret = 0;
	mutex_lock(&vfs_lock);

	if (open_fds[gfd])
		ret = open_fds[gfd]->flags_operating_mode;

	mutex_unlock(&vfs_lock);

	return ret;
}

int vfs_set_operating_mode(int gfd, uint32_t flags_operating_mode)
{
	mutex_lock(&vfs_lock);

	open_fds[gfd]->flags_operating_mode = flags_operating_mode;

	mutex_unlock(&vfs_lock);

	return 0;
}

/*
 * @brief this function will link a process fd (localfd) with the table of global fd (fd).
 * In addition, the function will increment the ref_count.
 */
void vfs_link_fd(int fd, int gfd)
{
	ASSERT(mutex_is_locked(&vfs_lock));

	current()->pcb->fd_array[fd] = gfd;

	vfs_inc_ref(gfd);
}

/*
 * @brief  This function will clone file descriptors.
 * @param  The file descriptor mappings to clone.
 * @param  The clone file descriptor mappings.
 * @return 0 if success, -1 if failure.
 *
 */
int vfs_clone_fd(int *fd_src, int *fd_dst)
{
	unsigned i;

	mutex_lock(&vfs_lock);

	/* All file descriptors are duplicated at the process level.
	 * However, the global descriptor remains the same in all cases.
	 */
	for (i = 0; i < FD_MAX; i++) {
		/* If invalid fd */
		if ((fd_src[i] < 0) || (open_fds[fd_src[i]] == NULL)) {
			fd_dst[i] = -1;
			continue;
		}

		fd_dst[i] = fd_src[i];
		vfs_inc_ref(fd_src[i]);
	}

	mutex_unlock(&vfs_lock);

	return 0;
}

/**************************** Syscall implementation ****************************/

/* Low Level read */
static int do_read(int fd, void *buffer, size_t count)
{
	int gfd;
	int ret;

	/* FIXME Max size of buffer */
	if (!buffer || count < 0) {
		LOG_ERROR("Invalid inputs\n");
		return -EINVAL;
	}

	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	/* FIXME: As for now the do_read/do_open only
	 * support regular file VFS_TYPE_FILE, VFS_TYPE_IO and pipes
	 */
	if (open_fds[gfd]->type == VFS_TYPE_DIR) {
		mutex_unlock(&vfs_lock);
		return -EISDIR;
	}

	if (!vfs_is_valid_gfd(gfd)) {
		mutex_unlock(&vfs_lock);
		return -EINVAL;
	}

	if (!open_fds[gfd]->fops->read) {
		LOG_ERROR("No fops read\n");
		mutex_unlock(&vfs_lock);
		return -EBADF;
	}

	mutex_unlock(&vfs_lock);

	ret = open_fds[gfd]->fops->read(gfd, buffer, count);

	return ret;
}

/* Low Level write */
static int do_write(int fd, const void *buffer, size_t count)
{
	int gfd;
	int ret;

	/* FIXME Max size of buffer */
	if (!buffer || count < 0) {
		LOG_ERROR("Invalid inputs\n");
		return -EINVAL;
	}

	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	if (!vfs_is_valid_gfd(gfd)) {
		mutex_unlock(&vfs_lock);
		return -EINVAL;
	}

	/* FIXME: As for now the do_read/do_open only
	 * support regular file VFS_TYPE_FILE, VFS_TYPE_IO and pipes
	 */
	if (open_fds[gfd]->type == VFS_TYPE_DIR) {
		mutex_unlock(&vfs_lock);
		return -EISDIR;
	}

	if (!open_fds[gfd]->fops->write) {
		mutex_unlock(&vfs_lock);
		return -EBADF;
	}

	mutex_unlock(&vfs_lock);

	ret = open_fds[gfd]->fops->write(gfd, buffer, count);

	return ret;
}

/* Low Level mmap */
static int do_mmap(int fd, addr_t virt_addr, uint32_t page_count, off_t offset)
{
	int gfd;
	struct file_operations *fops;

	/* Get the fops associated to the file descriptor. */
	gfd = vfs_get_gfd(fd);
	if (-1 == gfd) {
		printk("%s: could not get global fd.\n", __func__);
		return -EBADF;
	}

	mutex_lock(&vfs_lock);
	fops = vfs_get_fops(gfd);
	if (!fops) {
		printk("%s: could not get device fops.\n", __func__);
		mutex_unlock(&vfs_lock);
		return -EBADF;
	}

	mutex_unlock(&vfs_lock);

	if (!fops->mmap) {
		printk("%s: device doesn't support mmap.\n", __func__);
		return -EACCES;
	}

	/* Call the mmap fops that will do the actual mapping. */
	return (long) fops->mmap(fd, virt_addr, page_count, offset);
}

/* Low Level mmap - Anonymous case */
static int do_mmap_anon(int fd, addr_t virt_addr, uint32_t page_count, off_t offset)
{
	uint32_t page;
	pcb_t *pcb;
	int i;

	if (offset != 0) {
		printk("%s: Offset should be 0 with MAP_ANONYMOUS flag\n", __func__);
		return -EINVAL;
	}

	pcb = current()->pcb;

	/* Start is smaller or NULL than initial virt_addr pointer */
	if (virt_addr < pcb->next_anon_start)
		virt_addr = pcb->next_anon_start;

	for (i = 0; i < page_count; i++) {
		page = get_free_page();
		BUG_ON(!page);

		create_mapping(pcb->pgtable, virt_addr + (i * PAGE_SIZE), page, PAGE_SIZE, false);
		add_page_to_proc(pcb, phys_to_page(page));
	}

	memset((void *) virt_addr, 0, page_count * PAGE_SIZE);

	/* WARNIMG - This is a simple/basic way to set the start virtual address:
	             It only increment the start address after each mmap call, no algorithm
	             to search for available spaces.
	*/
	pcb->next_anon_start = virt_addr + (page_count * PAGE_SIZE);

	return virt_addr;
}

/**
 * @brief Low level stat implementation.
 */
static long do_stat(const char *path, struct stat *st)
{
	int ret;

	memset(st, 0, sizeof(*st));

	mutex_lock(&vfs_lock);

	/* FIXME Find the correct mount point with the path */
	if (!registered_fs_ops[FS_FAT]) {
		mutex_unlock(&vfs_lock);
		return -ENOENT;
	}

	if (!registered_fs_ops[FS_FAT]->stat) {
		mutex_unlock(&vfs_lock);
		return -ENOENT;
	}

	ret = registered_fs_ops[FS_FAT]->stat(path, st);

	mutex_unlock(&vfs_lock);

	return ret;
}

/**
 * @brief Function to convert stat to stat64 for ARM32 compatibility.
 */
static struct stat64 stat_to_stat64(struct stat *st)
{
	return (struct stat64) {
		.st_dev = st->st_dev,
		.__st_ino = st->st_ino,
		.st_mode = st->st_mode,
		.st_nlink = st->st_nlink,
		.st_uid = st->st_uid,
		.st_gid = st->st_gid,
		.st_rdev = st->st_rdev,
		.st_size = st->st_size,
		.st_blksize = st->st_blksize,
		.st_blocks = st->st_blocks,
		.st_atime = st->st_atime,
		.st_atime_nsec = st->st_atime_nsec,
		.st_mtime = st->st_mtime,
		.st_mtime_nsec = st->st_mtime_nsec,
		.st_ctime = st->st_ctime,
		.st_ctime_nsec = st->st_ctime_nsec,
		.st_ino = st->st_ino,
	};
}

SYSCALL_DEFINE3(read, int, fd, void *, buffer, size_t, count)
{
	return do_read(fd, buffer, count);
}

/**
 * @brief This function writes a REGULAR FILE/FOLDER. It only support regular file, dirs and pipes
 */
SYSCALL_DEFINE3(write, int, fd, const void *, buffer, size_t, count)
{
	return do_write(fd, buffer, count);
}

/**
 * @brief This function opens a file. Not all file types are supported.
 */
SYSCALL_DEFINE3(open, const char *, filename, int, flags, mode_t, mode)
{
	int fd, gfd, ret = -1;
	uint32_t type;
	struct file_operations *fops;

	if (mode != 0) {
		LOG_WARNING("mode parameters isn't supported\n");
	}

	mutex_lock(&vfs_lock);

	/*
	 * Check if the entry is a /dev entry and associates the right fops.
	 */
	if (!strncmp(DEV_PREFIX, filename, DEV_PREFIX_LEN)) {
		fops = devclass_get_fops(filename + DEV_PREFIX_LEN, &type);
		if (!fops) {
			mutex_unlock(&vfs_lock);
			return -1;
		}
	} else if (!strcmp("dev", filename) || !strcmp("/dev", filename)) {
		fops = registered_fs_ops[FS_DEV];
	} else {
		/* FIXME: Should find the mounted point regarding the path */
		fops = registered_fs_ops[FS_FAT];
	}

	if (flags & O_DIRECTORY) {
		type = VFS_TYPE_DIR;
	} else {
		type = VFS_TYPE_FILE;
	}

	/* vfs_open is already clean fops and open_fds */
	fd = vfs_open(filename, fops, type);

	if (fd < 0) {
		/* fd already open */
		mutex_unlock(&vfs_lock);
		return -EBADF;
	}

	/* Get index of open_fds*/
	gfd = current()->pcb->fd_array[fd];

	vfs_set_open_mode(gfd, flags);

	/* The open() callback operation in the sub-layers must NOT suspend. */
	if (fops->open) {
		ret = fops->open(gfd, filename);
		if (ret != 0)
			goto open_failed;
	}

	mutex_unlock(&vfs_lock);

	return fd;

open_failed:

	free(open_fds[gfd]);
	open_fds[gfd] = NULL;
	mutex_unlock(&vfs_lock);

	return ret;
}

/**
 * @brief Simple openat implementation ignoring dirfd for aarch64.
 */
SYSCALL_DEFINE4(openat, int, dirfd, const char *, filename, int, flags, mode_t, mode)
{
	if (dirfd != AT_FDCWD) {
		LOG_WARNING("dirfd parameters isn't supported\n");
		return -ENOSYS;
	}

	return sys_do_open(filename, flags, mode);
}

/*
 * @brief gedetens64 read a directory entry which will be stored in a struct dirent entry.
 *        This is used for readdir on userspace.
 * @param fd This is the file descriptor provided as (DIR *) when doing opendir in the userspace.
 */
SYSCALL_DEFINE3(getdents64, int, fd, struct dirent *, buf, size_t, count)
{
	struct dirent *dirent;
	int gfd;

	/* Dirent d_name should be a flexible array, which is not the case here.
	   So ensure that the given dirent can actually our full dirent. */
	if (count < sizeof(struct dirent)) {
		return -EINVAL;
	}

	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	if (!vfs_is_valid_gfd(gfd)) {
		mutex_unlock(&vfs_lock);
		return -EBADF;
	}

	if (open_fds[gfd]->type != VFS_TYPE_DIR) {
		mutex_unlock(&vfs_lock);
		return -ENOTDIR;
	}

	if (!open_fds[gfd]->fops->readdir) {
		mutex_unlock(&vfs_lock);
		return -EBADF;
	}

	dirent = open_fds[gfd]->fops->readdir(gfd);
	if (!dirent) {
		mutex_unlock(&vfs_lock);
		return -EINVAL;
	}

	dirent->d_reclen = sizeof(struct dirent);
	memcpy(buf, dirent, dirent->d_reclen);

	mutex_unlock(&vfs_lock);

	return sizeof(struct dirent);
}

/*
 * @brief This function will close the the file descriptor. The close callback function associated to fops is called
 * only when refcount is equal to zero (no more reference on the gfd).
 * @param fd This is the local fd from the process' table.
 */
SYSCALL_DEFINE1(close, int, fd)
{
	pcb_t *pcb = current()->pcb;
	int gfd;

	if ((!pcb) || (fd < 0))
		return 0;

	mutex_lock(&vfs_lock);

	/* Get the global file descriptor */
	gfd = pcb->fd_array[fd];

	if (gfd < 0) {
		LOG_DEBUG("Was already freed\n");
		mutex_unlock(&vfs_lock);
		return 0;
	}

	if (!open_fds[gfd]) {
		mutex_unlock(&vfs_lock);
		return 0;
	}

	/* Decrement reference counter to keep track of open fds */
	ASSERT(open_fds[gfd]->ref_count > 0);
	open_fds[gfd]->ref_count--;

	ASSERT(open_fds[gfd]->ref_count >= 0);

	/* Unreference the process fd's table to -1 */
	pcb->fd_array[fd] = -1;

	/* The following part of the code maintain the global file descriptor (open fds) table */
	/* Free only when no one is using the file descriptor */

	if (!open_fds[gfd]->ref_count) {
		ASSERT(gfd > STDERR); /* Abnormal situation if we attempt to remove the std* file descriptors */

		/* The close() callback operation in the sub-layers must NOT suspend. */
		if (open_fds[gfd]->fops->close)
			open_fds[gfd]->fops->close(gfd);

		if (open_fds[gfd]->filename)
			free(open_fds[gfd]->filename);

		free(open_fds[gfd]);

		open_fds[gfd] = NULL;
	}

	mutex_unlock(&vfs_lock);

	return 0;
}

/**
 * @brief Simple dup3 implementation ignoring flags for aarch64.
 */
SYSCALL_DEFINE3(dup3, int, oldfd, int, newfd, int, flags)
{
	if (oldfd == newfd) {
		return -EINVAL;
	}

	if (flags != 0) {
		LOG_WARNING("Flags not supported.\n");
		return -ENOSYS;
	}

	return sys_do_dup2(oldfd, newfd);
}

/**
 * @brief dup2 creates a synonym of oldfd on newfd
 */
SYSCALL_DEFINE2(dup2, int, oldfd, int, newfd)
{
	if ((newfd < 0) || (newfd > MAX_FDS))
		return -EBADF;

	if ((oldfd < 0) || (oldfd > MAX_FDS))
		return -EBADF;

	mutex_lock(&vfs_lock);

	if (vfs_get_gfd(oldfd) < 0) {
		mutex_unlock(&vfs_lock);

		return -EBADF;
	}

	if (vfs_get_gfd(oldfd) != vfs_get_gfd(newfd))
		sys_do_close(newfd);

	vfs_link_fd(newfd, vfs_get_gfd(oldfd));

	mutex_unlock(&vfs_lock);

	return newfd;
}

/**
 * @brief This function will copy the oldfd and
 *		use the next unused fd in the process
 *		table.
 * @param File descriptor to copy.
 * @return A copy of the file descriptor.
 */
SYSCALL_DEFINE1(dup, int, oldfd)
{
#ifdef CONFIG_PROC_ENV
	int newfd;

	if (oldfd < 0 || oldfd > MAX_FDS)
		return -EBADF;

	mutex_lock(&vfs_lock);

	/* Retrieve a unused process fd */
	newfd = proc_new_fd(current()->pcb);
	if (newfd < 0) {
		mutex_unlock(&vfs_lock);
		return newfd;
	}

	/* Link it with the  */
	vfs_link_fd(newfd, vfs_get_gfd(oldfd));

	mutex_unlock(&vfs_lock);

	return newfd;
#else
	return 0;
#endif
}

/**
 * @brief On ARM32, stat64 syscall is called instead of stat.
 */
SYSCALL_DEFINE2(stat64, const char *, path, struct stat64 *, st)
{
	struct stat stat;
	long ret = 0;

	ret = do_stat(path, &stat);

	if (ret >= 0) {
		*st = stat_to_stat64(&stat);
	}

	return ret;
}

/**
 * @brief On ARM32, fstatat64 syscall is called instead of fstatat and stat in some condition.
 */
SYSCALL_DEFINE4(fstatat64, int, fd, const char *, path, struct stat64 *, st, int, flags)
{
	struct stat stat;
	int ret;

	if (fd != AT_FDCWD) {
		LOG_WARNING("fd parameters isn't supported\n");
		return -ENOSYS;
	}

	if (flags != 0) {
		LOG_WARNING("Flags not supported\n");
		return -ENOSYS;
	}

	ret = do_stat(path, &stat);

	if (ret >= 0) {
		*st = stat_to_stat64(&stat);
	}

	return ret;
}

/**
 * @brief On aarch64, newfstatat syscall is called for stat.
 */
SYSCALL_DEFINE4(newfstatat, int, fd, const char *, path, struct stat *, st, int, flags)
{
	if (fd != AT_FDCWD) {
		LOG_WARNING("fd parameters isn't supported\n");
		return -ENOSYS;
	}

	if (flags != 0) {
		LOG_WARNING("Flags not supported\n");
		return -ENOSYS;
	}

	return do_stat(path, st);
}

/**
 * An mmap() implementation in VFS.
 */
SYSCALL_DEFINE6(mmap, addr_t, start, size_t, length, int, prot, int, flags, int, fd, off_t, offset)
{
	uint32_t page_count;

	/* Page count to allocate to the current process to be able to map the desired region. */
	page_count = length / PAGE_SIZE;
	if (length % PAGE_SIZE != 0) {
		page_count++;
	}

	if (flags & MAP_ANONYMOUS)
		return do_mmap_anon(fd, start, page_count, offset);
	else
		return do_mmap(fd, start, page_count, offset);
}

/**
 * Works like mmap, but with the given offset been in page count.
 */
SYSCALL_DEFINE6(mmap2, addr_t, start, size_t, length, int, prot, int, flags, int, fd, off_t, pgoffset)
{
	return sys_do_mmap(start, length, prot, flags, fd, pgoffset * PAGE_SIZE);
}

SYSCALL_DEFINE3(ioctl, int, fd, unsigned long, cmd, unsigned long, args)
{
	int rc, gfd;
	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	if (!vfs_is_valid_gfd(gfd)) {
		mutex_unlock(&vfs_lock);
		return -EINVAL;
	}

	if (open_fds[gfd]->fops->ioctl)
		rc = open_fds[gfd]->fops->ioctl(fd, cmd, args);
	else {
		rc = -EPERM;
	}

	mutex_unlock(&vfs_lock);

	return rc;
}

/*
 * Implementation of standard lseek() syscall. It depends on the underlying
 * device operations.
 */
SYSCALL_DEFINE3(lseek, int, fd, off_t, off, int, whence)
{
	int rc, gfd;
	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	if (!vfs_is_valid_gfd(gfd)) {
		mutex_unlock(&vfs_lock);
		return -EINVAL;
	}

	if (open_fds[gfd]->fops->lseek)
		rc = open_fds[gfd]->fops->lseek(fd, off, whence);
	else
		rc = 0; /* Nothing if no specific callback found. */

	mutex_unlock(&vfs_lock);

	return rc;
}

/**
 * @brief Implementation of llseek syscall for ARM32 which use it instead of lseek.
 */
SYSCALL_DEFINE5(_llseek, int, fd, unsigned long, offset_high, unsigned long, offset_low, off_t *, result, unsigned, whence)
{
	off_t offset = ((off_t) offset_high << 32) | offset_low;
	off_t ret;

	ret = sys_do_lseek(fd, offset, whence);

	if (ret >= 0) {
		/* New offset is returned in the 64 bits of result. */
		*result = ret;
		ret = 0;
	}

	return ret;
}

/*
 * Implementation of the writev syscall
 */
SYSCALL_DEFINE3(writev, unsigned long, fd, const struct iovec *, vec, unsigned long, vlen)
{
	int i;
	int ret = 0;
	int total = 0;

	for (i = 0; i < vlen; i++) {
		ret = do_write(fd, (const void *) vec[i].iov_base, vec[i].iov_len);
		if (ret < 0) {
			break;
		} else if ((ret >= 0) && (ret < vec[i].iov_len)) {
			total += ret;
			break;
		}

		total += ret;
	}

	if (total == 0)
		return ret;
	else
		return total;
}

SYSCALL_DEFINE3(readv, unsigned long, fd, const struct iovec *, vec, unsigned long, vlen)
{
	int i;
	int ret = 0;
	int total = 0;

	for (i = 0; i < vlen; i++) {
		ret = do_read(fd, vec[i].iov_base, vec[i].iov_len);
		if (ret < 0) {
			break;
		} else if ((ret >= 0) && (ret < vec[i].iov_len)) {
			total += ret;
			break;
		}

		total += ret;
	}

	if (total == 0)
		return ret;
	else
		return total;
}

static void vfs_gfd_init(void)
{
	memset(open_fds, 0, MAX_FDS * sizeof(struct fd *));

	/* Basic file descriptors */
	open_fds[STDIN] = (struct fd *) malloc(sizeof(struct fd));
	open_fds[STDOUT] = (struct fd *) malloc(sizeof(struct fd));
	open_fds[STDERR] = (struct fd *) malloc(sizeof(struct fd));

	if (!open_fds[STDIN] || !open_fds[STDOUT] || !open_fds[STDERR]) {
		printk("%s: failed to allocate memory\n", __func__);
		kernel_panic();
	}

	open_fds[STDIN]->val = STDIN;
	open_fds[STDIN]->type = VFS_TYPE_IO;
	open_fds[STDIN]->fops = &console_fops;

	open_fds[STDOUT]->val = STDOUT;
	open_fds[STDOUT]->type = VFS_TYPE_IO;
	open_fds[STDOUT]->fops = &console_fops;

	open_fds[STDERR]->val = STDERR;
	open_fds[STDERR]->type = VFS_TYPE_IO;
	open_fds[STDERR]->fops = &console_fops;

	/* Ref counter updated to 1 on init */
	open_fds[STDERR]->ref_count = 1;
	open_fds[STDIN]->ref_count = 1;
	open_fds[STDOUT]->ref_count = 1;
}

void vfs_init(void)
{
#ifdef CONFIG_FS_FAT
	/* FIXME: Handle multiple mounting points  */
	if (!registered_fs_ops[FS_FAT]) {
		registered_fs_ops[FS_FAT] = register_fat();

		/* FIXME Mount root */
		registered_fs_ops[FS_FAT]->mount("");
	}
#endif
	if (!registered_fs_ops[FS_DEV]) {
		registered_fs_ops[FS_DEV] = register_devfs();
	}
	mutex_init(&vfs_lock);

	vfs_gfd_init();
}
