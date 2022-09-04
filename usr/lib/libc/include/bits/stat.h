/* copied from kernel definition, but with padding replaced
 * by the corresponding correctly-sized userspace types. */

#define FILENAME_SIZE 	256

struct stat {
        
#if 0 /* Not yet available */
dev_t st_dev;
	int __st_dev_padding;
	long __st_ino_truncated;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	int __st_rdev_padding;
	off_t st_size;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	ino_t st_ino;
#endif /* 0 */

	char st_name[FILENAME_SIZE];	/* Filename */
	unsigned long st_size; 		/* Size of file */
	time_t st_mtim;				/* Time of last modification in sec*/
	unsigned char st_flags;		/* Regular file flag (not supported on fat) */
	mode_t st_mode;		        /* Protection not used (not supported on fat) */
};
