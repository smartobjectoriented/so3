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

#if 0
#define DEBUG
#endif

#include <vfs.h>
#include <process.h>
#include <heap.h>
#include <errno.h>
#include <process.h>
#include <mutex.h>
#include <string.h>
#include <dirent.h>
#include <console.h>

#include <fat/fat.h>

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
 * @brief Get a new available global file descriptor (gfd)
 *
 * vfs_lock must be hold.
 */
static int vfs_new_gfd(void)
{
	int i;

	ASSERT(mutex_is_locked(&vfs_lock));

	for (i = 0; i < FD_MAX ; i++)
		if (open_fds[i] == NULL)
			return i;
	return -1;
}

/*
 * @brief Get the refcount of a specific global fd.
 *
 */
int vfs_refcount(int gfd) {
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

/* @brief This function will increment the fd ref_count
 *		to keep track on how many instance are
 *		using the the file descriptor
 * @return the new refcount
 *
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
struct file_operations *vfs_get_fops(uint32_t gfd) {

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
		set_errno(ENFILE);
		goto vfs_open_failed;
	}

	open_fds[gfd] = (struct fd *) malloc(sizeof(struct fd));

	if (!open_fds[gfd]) {
		printk("%s: failed to allocate memory\n", __func__);
		set_errno(ENOMEM);
		goto vfs_open_failed;
	}

	memset(open_fds[gfd], 0, sizeof(struct fd));

	/* Store the filename */
	if (filename) {
		open_fds[gfd]->filename = malloc(strlen(filename)+1);
		if (!open_fds[gfd]) {
			printk("%s: failed to allocate memory\n", __func__);
			set_errno(ENOMEM);
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
	return -1;
}

uint32_t vfs_get_open_mode(int gfd)
{
	ASSERT(mutex_is_locked(&vfs_lock));

	if (open_fds[gfd]) {
		return open_fds[gfd]->flags_open;
	}

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
	uint32_t ret;

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
	uint32_t ret;
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
 * Duplicate a gfd.
 *
 * vfs_lock must be hold.
 */
static int vfs_copy_gfd(int gfd_src, int gfd_dst)
{
	struct fd *fdesc;

	ASSERT(mutex_is_locked(&vfs_lock));

	fdesc = (struct fd *) malloc(sizeof(struct fd));
	if (!fdesc) {
		printk("%s: failed to allocate memory\n", __func__);
		return -1;
	}

	memcpy(fdesc, open_fds[gfd_src], sizeof(struct fd));
	open_fds[gfd_dst] = fdesc;

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
	int newgfd;
	int rc;

	mutex_lock(&vfs_lock);

	/* The standard streams are not cloned. Only the reference is 
	 * incremented
	 */

	for (i = 0; i < STDERR + 1; i++) {
		fd_dst[i] = fd_src[i];
		vfs_inc_ref(fd_src[i]);
	}

	for (i = STDERR + 1; i < FD_MAX; i++) {
		/* If invalid fd */
		if ((fd_src[i] < 0) || (open_fds[fd_src[i]] == NULL)) {
			fd_dst[i] = -1;
			continue;
		}

		/* In the case of pipe, the (global) file descriptor must remain
		 * the same.
		 * The clone() callback operation in the sub-layers must NOT suspend.
		 */
		if (open_fds[fd_src[i]]->fops->clone != NULL)
			open_fds[fd_src[i]]->fops->clone(fd_src[i]);

		if (open_fds[fd_src[i]]->type == VFS_TYPE_PIPE) {

			fd_dst[i] = fd_src[i];
			vfs_inc_ref(fd_src[i]);

		} else {

			newgfd = vfs_new_gfd();
			if (newgfd < 0) {
				mutex_unlock(&vfs_lock);
				return newgfd;
			}

			if ((rc = vfs_copy_gfd(fd_src[i], newgfd))) {
				mutex_unlock(&vfs_lock);
				return rc;
			}

			fd_dst[i] = newgfd;
		}
	}

	mutex_unlock(&vfs_lock);

	return 0;
}

/**************************** Syscall implementation ****************************/
 
int do_read(int fd, void *buffer, int count)
{
	int gfd;
	int ret;

	/* FIXME Max size of buffer */
	if (!buffer || count < 0) {
		DBG("Invalid inputs\n");
		set_errno(EINVAL);
		return -1;
	}

	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	/* FIXME: As for now the do_read/do_open only
	 * support regular file VFS_TYPE_FILE, VFS_TYPE_IO and pipes
	 */
	if (open_fds[gfd]->type == VFS_TYPE_DIR) {
		set_errno(EISDIR);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	if (!vfs_is_valid_gfd(gfd)) {
		set_errno(EINVAL);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	if (!open_fds[gfd]->fops->read) {
		DBG("No fops read\n");
		set_errno(EBADF);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	mutex_unlock(&vfs_lock);

	ret = open_fds[gfd]->fops->read(gfd, buffer, count);

	return ret;
}

/**
 * @brief This function writes a REGULAR FILE/FOLDER. It only support regular file, dirs and pipes
 */
int do_write(int fd, const void *buffer, int count)
{
	int gfd;
	int ret;

	/* FIXME Max size of buffer */
	if (!buffer || count < 0) {
		DBG("Invalid inputs\n");
		set_errno(EINVAL);
		return -1;
	}

	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	if (!vfs_is_valid_gfd(gfd)) {
		set_errno(EINVAL);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	/* FIXME: As for now the do_read/do_open only
	 * support regular file VFS_TYPE_FILE, VFS_TYPE_IO and pipes
	 */
	if (open_fds[gfd]->type == VFS_TYPE_DIR) {
		set_errno(EISDIR);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	if (!open_fds[gfd]->fops->write) {
		set_errno(EBADF);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	mutex_unlock(&vfs_lock);

	ret = open_fds[gfd]->fops->write(gfd, buffer, count);

	return ret;
}

/**
 * @brief This function opens a file. Not all file types are supported.
 */
int do_open(const char *filename, int flags)
{
	int fd, gfd, ret = -1;
	uint32_t type;
	struct file_operations *fops;

	mutex_lock(&vfs_lock);

	/* FIXME: Should find the mounted point regarding the path */
	fops = registered_fs_ops[0];

	if (flags & O_DIRECTORY)
		type = VFS_TYPE_DIR;
	else
		type = VFS_TYPE_FILE;

	/* vfs_open is already clean fops and open_fds */
	fd = vfs_open(filename, fops, type);

	if (fd < 0) {
		/* fd already open */
		set_errno(EBADF);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	/* Get index of open_fds*/
	gfd = current()->pcb->fd_array[fd];

	vfs_set_open_mode(gfd, flags);

	/* The open() callback operation in the sub-layers must NOT suspend. */
	if (fops->open && fops->open(gfd, filename)) {
		ret = -1;
		goto open_failed;
	}

	/*
	 * Check if the entry is a /dev entry and associates the right fops.
	 */

	if (!strncmp(DEV_PREFIX, filename, DEV_PREFIX_LEN)) {

		fops = devclass_get_fops(filename + DEV_PREFIX_LEN, &type);
		if (!fops) {
			mutex_unlock(&vfs_lock);
			return -1;
		}

		open_fds[gfd]->fops = fops;
	}

	mutex_unlock(&vfs_lock);

	return fd;

open_failed:

	free(open_fds[gfd]);
	open_fds[gfd] = NULL;
	mutex_unlock(&vfs_lock);

	return ret;
}

/*
 * @brief readdir read a directory entry which will be stored in a struct dirent entry
 * @param fd This is the file descriptor provided as (DIR *) when doing opendir in the userspace.
 */
int do_readdir(int fd, char *buf, int len)
{
	struct dirent *dirent;
	int gfd;

	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	if (!vfs_is_valid_gfd(gfd)) {
		set_errno(EBADF);
		mutex_unlock(&vfs_lock);
		return 0;
	}

	if (open_fds[gfd]->type != VFS_TYPE_DIR) {
		set_errno(ENOTDIR);
		mutex_unlock(&vfs_lock);
		return 0;
	}

	if (!open_fds[gfd]->fops->readdir) {
		set_errno(EBADF);
		mutex_unlock(&vfs_lock);
		return 0;
	}

	dirent = open_fds[gfd]->fops->readdir(gfd);
	if (!dirent) {
		mutex_unlock(&vfs_lock);
		return 0;
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
void do_close(int fd)
{
	pcb_t *pcb = current()->pcb;
	int gfd;

	if ((!pcb) || (fd < 0))
		return;

	mutex_lock(&vfs_lock);

	/* Get the global file descriptor */
	gfd = pcb->fd_array[fd];

	if (gfd < 0) {
		DBG("Was already freed\n");
		mutex_unlock(&vfs_lock);
		return;
	}

	if (!open_fds[gfd]) {
		mutex_unlock(&vfs_lock);
		return ;
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

		/* For /dev entries, we attach the default callback operations of the regular filesystem */
		if (open_fds[gfd]->filename && !strncmp(DEV_PREFIX, open_fds[gfd]->filename, DEV_PREFIX_LEN))
			/* Back to the filesystem medium */
			open_fds[gfd]->fops = registered_fs_ops[0];

		/* The close() callback operation in the sub-layers must NOT suspend. */
		if (open_fds[gfd]->fops->close)
			open_fds[gfd]->fops->close(gfd);

		if (open_fds[gfd]->filename)
			free(open_fds[gfd]->filename);

		free(open_fds[gfd]);

		open_fds[gfd] = NULL;
	}

	mutex_unlock(&vfs_lock);
}

/**
 * @brief dup2 creates a synonym of oldfd on newfd
 *
 */
int do_dup2(int oldfd, int newfd)
{
	if ((newfd < 0) || (newfd > MAX_FDS))
		return -EBADF;


	if ((oldfd < 0) || (oldfd > MAX_FDS))
		return -EBADF;

	mutex_lock(&vfs_lock);

	if (vfs_get_gfd(oldfd) < 0) {
		set_errno(EBADF);
		mutex_unlock(&vfs_lock);

		return -1;
	}

	if (vfs_get_gfd(oldfd) !=  vfs_get_gfd(newfd))
		do_close(newfd);


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
int do_dup(int oldfd)
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

int do_stat(const char *path, struct stat *st)
{
	int ret;

	mutex_lock(&vfs_lock);

	/* FIXME Find the correct mount point with the path */
	if (!registered_fs_ops[0]) {
		set_errno(ENOENT);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	if (!registered_fs_ops[0]->stat) {
		set_errno(ENOENT);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	ret = registered_fs_ops[0]->stat(path, st);

	mutex_unlock(&vfs_lock);

	return ret;
}

/**
 * An mmap() implementation in VFS.
 */
void *do_mmap(addr_t start, size_t length, int prot, int fd, off_t offset)
{
	int gfd;
	uint32_t page_count;
	struct file_operations *fops;

	/* Get the fops associated to the file descriptor. */

	gfd = vfs_get_gfd(fd);
	if (-1 == gfd) {
		printk("%s: could not get global fd.", __func__);
		return NULL;
	}

	mutex_lock(&vfs_lock);
	fops = vfs_get_fops(gfd);
	if (!fops) {
		printk("%s: could not get the framebuffer fops.", __func__);
		mutex_unlock(&vfs_lock);
		return NULL;
	}

	mutex_unlock(&vfs_lock);

	/* Page count to allocate to the current process to be able to map the desired region. */
	page_count = length / PAGE_SIZE;
	if (length % PAGE_SIZE != 0) {
		page_count++;
	}

	/* Call the mmap fops that will do the actual mapping. */
	return fops->mmap(fd, start, page_count);
}

int do_ioctl(int fd, unsigned long cmd, unsigned long args)
{
	int rc, gfd;
	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	if (!vfs_is_valid_gfd(gfd)) {
		set_errno(EINVAL);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	if (open_fds[gfd]->fops->ioctl)
		rc = open_fds[gfd]->fops->ioctl(fd, cmd, args);
	else {
		set_errno(EPERM);
		rc = -1;
	}

	mutex_unlock(&vfs_lock);

	return rc;
}

/*
 * Implementation of standard lseek() syscall. It depends on the underlying
 * device operations.
 */
off_t do_lseek(int fd, off_t off, int whence) {
	int rc, gfd;
	mutex_lock(&vfs_lock);

	gfd = vfs_get_gfd(fd);

	if (!vfs_is_valid_gfd(gfd)) {
		set_errno(EINVAL);
		mutex_unlock(&vfs_lock);
		return -1;
	}

	if (open_fds[gfd]->fops->lseek)
		rc = open_fds[gfd]->fops->lseek(fd, off, whence);
	else
		rc = 0; /* Nothing if no specific callback found. */

	mutex_unlock(&vfs_lock);

	return rc;
}

/*
 * Implementation of the fcntl syscall
 */
int do_fcntl(int fd, unsigned long cmd, unsigned long args) {

	/* Not yet implemented */

	return 0;
}

static void vfs_gfd_init(void)
{
	memset(open_fds, 0, sizeof(struct fd));

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
	if (!registered_fs_ops[0]) {
		registered_fs_ops[0] = register_fat();

		/* FIXME Mount root */
		registered_fs_ops[0]->mount("");
	}
#endif
	mutex_init(&vfs_lock);

	vfs_gfd_init();
}
