/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
 * Copyright (C) 2016-2017 Alexandre Malki <alexandre.malki@heig-vd.ch>
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

#include <common.h>
#include <errno.h>

#include <fat/fat.h>
#include <fat/ff.h>
#include <dirent.h>
#include <heap.h>

#include <device/timer.h>
#include <device/serial.h>

#define TYPE_FILE	0
#define TYPE_FOLDER	1

#define MOUNT_LATER	0
#define MOUNT_NOW	1

/*
 * The epoch of FAT timestamp is 1980.
 *     :  bits :     value
 * date:  0 -  4: day	(1 -  31)
 * date:  5 -  8: month	(1 -  12)
 * date:  9 - 15: year	(0 - 127) from 1980
 * time:  0 -  4: sec	(0 -  29) 2sec counts
 * time:  5 - 10: min	(0 -  59)
 * time: 11 - 15: hour	(0 -  23)
 */
#define SECS_PER_MIN	60
#define SECS_PER_HOUR	(60 * 60)
#define SECS_PER_DAY	(SECS_PER_HOUR * 24)
/* days between 1.1.70 and 1.1.80 (2 leap days) */
#define DAYS_DELTA	(365 * 10 + 2)
/* 120 (2100 - 1980) isn't leap year */
#define YEAR_2100	120
#define IS_LEAP_YEAR(y)	(!((y) & 3) && (y) != YEAR_2100)

/* Linear day numbers of the respective 1sts in non-leap years. */
static time_t days_in_year[] = {
	/* Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec */
	0,   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 0, 0, 0,
};


struct stream {
	struct dirent entry;
	DIR	dir_ctx;
};

struct fat_entry {
	/* Type of entry */
	int tentry;
	union {
		FIL file;
		struct stream dir;
	} entry;
};

struct fat_mount {
	int mounted;
	FATFS mp;
};

static struct fat_mount volumes[4];

static int open_fat_file(int fd, const char *path, struct fat_entry *ptrent)
{
	uint32_t flags = vfs_get_open_mode(fd);
	uint8_t fat_flags = 0;
	int rc;

	flags |= vfs_get_access_mode(fd);
	flags |= vfs_get_operating_mode(fd);

	if (O_TRUNC & flags) {
		fat_flags |= FA_CREATE_ALWAYS;
	} else if (O_CREAT & flags) {
		fat_flags |= FA_CREATE_NEW;
	} else if (O_APPEND) {
		fat_flags |= FA_OPEN_EXISTING;
	}

	/* At least */
	fat_flags |= FA_READ;

	if (O_WRONLY & flags) {
		fat_flags |= FA_WRITE;
	} else if (O_RDWR & flags) {
		fat_flags |= (FA_WRITE | FA_READ);
	}

	if (O_EXCL & flags) {
		fat_flags |= FA_CREATE_ALWAYS;
	}

	if ((rc = f_open(&ptrent->entry.file, path, fat_flags))) {
		return -rc;
	}


	return 0;
}

/* @brief: This remove the last occurence the char '/'
 * at the end of a string */
static void last_delim_remove(char *path)
{
	int len = strlen(path);

	if (len <= 0) {
		return;
	}

	while (path[len] == '/')  {
		if (&path[len] == path) {
			break;
		}

		path[len] = '\0';
		len--;
	}
}

/* This function was in part taken from linux */
static void time_fat_fat2so3(unsigned short date, unsigned short time, struct timespec *ts)
{
	time_t second, day, leap_day, month, year;

	year  = date >> 9;
	month = max(1, (date >> 5) & 0xf);
	day   = max(1, date & 0x1f) - 1;

	leap_day = (year + 3) / 4;
	if (year > YEAR_2100)		/* 2100 isn't leap year */
		leap_day--;
	if (IS_LEAP_YEAR(year) && month > 2)
		leap_day++;

	second =  (time & 0x1f) << 1;
	second += ((time >> 5) & 0x3f) * SECS_PER_MIN;
	second += (time >> 11) * SECS_PER_HOUR;
	second += (year * 365 + leap_day
		   + days_in_year[month] + day
		   + DAYS_DELTA) * SECS_PER_DAY;

#if 0
	if (!sbi->options.tz_set)
		second += sys_tz.tz_minuteswest * SECS_PER_MIN;
	else
		second -= sbi->options.time_offset * SECS_PER_MIN;
#endif

		ts->tv_sec = second;
		ts->tv_nsec = 0;
}

int fat_read(int fd, void *buffer, int count)
{
	struct fat_entry *ptrent = (struct fat_entry *) vfs_get_priv(fd);
	int rc = 0;
	int bread = 0;

	if (!ptrent) {
		DBG("Error while reading fd %d\n", fd);
		return -1;
	}

	if (ptrent->tentry != TYPE_FILE) {
		return -EINVAL;
	}

	if ((rc = f_read(&ptrent->entry.file, buffer, count, (unsigned *) &bread))) {
		return -rc;
	}

	return bread;
}

int fat_write(int fd, const void *buffer, int count)
{
	struct fat_entry *ptrent = (struct fat_entry *) vfs_get_priv(fd);
	int rc = 0;
	int bwritten = 0;

	if (!ptrent) {
		DBG("Error while writing fd %d\n", fd);
		return -1;
	}

	if (ptrent->tentry != TYPE_FILE) {
		return -EINVAL;
	}

	if ((rc = f_write(&ptrent->entry.file, buffer, count, (unsigned *) &bwritten))) {
		return -rc;
	}

	rc = f_sync(&ptrent->entry.file);

	return bwritten;
}

/* @brief This function will open a file or folder depending on the
 *		kind of type provided
 *
 */
int fat_open(int fd, const char *path)
{
	struct fat_entry *ptrent;
	int rc;
	uint32_t open_flags = vfs_get_open_mode(fd);

	ptrent = (struct fat_entry *) malloc(sizeof(struct fat_entry));
	if (!ptrent) {
		printk("%s: heap overflow...\n", __func__);
		kernel_panic();
	}

	switch (open_flags & O_DIRECTORY){
		case O_DIRECTORY:
			/* This is a directory */
			last_delim_remove((char *) path);
			if ((rc = f_opendir(&ptrent->entry.dir.dir_ctx, path))) {
				goto open_fail;
			}

			ptrent->tentry =  TYPE_FOLDER;
			break;
		default:
			/* Default is a file */
			if ((rc = open_fat_file(fd, path, ptrent))) {
				goto open_fail;
			}

			ptrent->tentry =  TYPE_FILE;
			break;
	}

	vfs_set_priv(fd, (void *)ptrent);
	return 0;

open_fail:
	free(ptrent);
	return -rc;
}

int fat_close(int fd)
{
	int rc;
	struct fat_entry *ptrent = (struct fat_entry *) vfs_get_priv(fd);

	switch (ptrent->tentry) {
		case TYPE_FILE:
			if ((rc = f_close(&ptrent->entry.file))) {
				return -rc;
			}
			break;
		case TYPE_FOLDER:
			if ((rc = f_closedir(&ptrent->entry.dir.dir_ctx))) {
				return -rc;
			}
			break;
		default:
			return -EBADFD;
	}

	free(ptrent);
	return 0;
}

int fat_mount(const char *mount_point)
{
	int i;
	int rc = 0;

	for(i = 0; i < ARRAY_SIZE(volumes); i++) {
		if (!volumes[i].mounted) {
			if((rc = f_mount(&volumes[i].mp, mount_point, MOUNT_NOW))) {
				DBG("Error %d while mounting volume %s\n", rc, mount_point);
				return -rc;
			}

			return 0;
		}
	}

	/* FIXME : find appropriate errno */
	return -1;
}

int fat_unmount(const char *mount_point)
{
	return 0;
}

struct dirent *fat_readdir(int fd)
{
	struct fat_entry *ptrent = (struct fat_entry *) vfs_get_priv(fd);
	struct dirent *dent;
	DIR *tmp_dir;
	FILINFO fno;
	char *fn;
	int rc;

	if (ptrent->tentry != TYPE_FOLDER)
		return NULL;

	dent = &ptrent->entry.dir.entry;
	tmp_dir = &ptrent->entry.dir.dir_ctx;

	rc = f_readdir(tmp_dir, &fno);

	/* Error while readding dir */
	if (rc)
		return NULL;

	/* No more file/folder */
	DBG("altname = %s , fname = %s\n", fno.altname, fno.fname);
	if (fno.fname[0] == 0)
		return NULL;

	/* Depending if altname or fname is used */
	fn = *fno.fname ? fno.fname : fno.altname;
	memset(dent, 0, sizeof(*dent));

	if (fno.fattrib & AM_DIR)
		dent->d_type = DT_DIR;
	else
		dent->d_type = DT_REG;

	strcpy(dent->d_name, fn);

	return (void *) dent;
}

int fat_unlink(int fd, const char *path)
{
	return 0;
}

int fat_stat(const char *path, struct stat *st)
{
	FILINFO finfo;
	struct timespec tm;
	int res;

	if (!path || !st) {
		set_errno(EINVAL);
		return -1;
	}

	memset(st, 0, sizeof(struct stat));

	if ((res = f_stat(path, &finfo))) {
		set_errno(EEXIST);
	}

	time_fat_fat2so3(finfo.fdate, finfo.ftime, &tm);
	st->st_mtim = tm.tv_sec;
	strcpy(st->st_name, path);
	st->st_size = finfo.fsize;


	return res;
}

/* Request an ioctl from user space */
static int fat_ioctl(int fd, unsigned long cmd, unsigned long args)
{
	int rc;

	switch (cmd) {
		case TIOCGWINSZ:
			rc = serial_gwinsize((struct winsize *) args);
			break;
		default:
			rc = -1;
			break;
	}

	return rc;
}

/**
 * lseek implementation for FAT filesystem
 * @whence can have the following value:
 *
 * - SEEK_SET        seek relative to beginning of file
 * - SEEK_CUR        seek relative to current file position
 * - SEEK_END        seek relative to end of file
 * - SEEK_DATA       seek to the next data
 * - SEEK_HOLE       seek to the next hole
 *
 * Upon  successful  completion,  lseek() returns the resulting offset location as measured in bytes
 * from the beginning of the file.  On error, the value (off_t) -1 is returned and errno is set to indicate the error.
 *
 */
static off_t fat_lseek(int fd, off_t off, int whence) {
	struct fat_entry *ptrent = (struct fat_entry *) vfs_get_priv(fd);
	off_t ret;
	uint32_t eof, cur;

	/* Get the size of the file (eof) */
	eof = ptrent->entry.file.obj.objsize;

	/* Get the current position */
	cur = ptrent->entry.file.fptr;

	switch (whence) {
		case SEEK_END:
			off += eof;
			break;

		case SEEK_CUR:
			/*
			 * Here we special-case the lseek(fd, 0, SEEK_CUR)
			 * position-querying operation.  Avoid rewriting the "same"
			 * f_pos value back to the file because a concurrent read(),
			 * write() or lseek() might have altered it
			 */
			if (off == 0)
				return ptrent->entry.file.fptr;

			off += cur;
			if (off >= eof) {
				set_errno(EINVAL);
				return -1;
			}
			break;

		case SEEK_DATA:
			if (off >= eof)
				return -ENXIO;
			break;

		case SEEK_HOLE:
			/*
			 * There is a virtual hole at the end of the file, so as long as
			 * offset isn't i_size or larger, return i_size.
			 */
			if (off >= eof)
				return -ENXIO;
			off = eof;
			break;
	}

	ret = f_lseek(&ptrent->entry.file, off);
	if (ret) {
		set_errno(EINVAL);
		return (off_t) -1;
	}

	return off;
}

static struct file_operations fatops = {
	.open = fat_open,
	.close = fat_close,
	.read = fat_read,
	.write = fat_write,
	.mount = fat_mount,
	.readdir = fat_readdir,
	.stat = fat_stat,
	.ioctl = fat_ioctl,
	.lseek = fat_lseek
};

struct file_operations *register_fat(void)
{
	return &fatops;
}


