#include "stdio_impl.h"
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <syscall.h>

FILE *fopen(const char *restrict filename, const char *restrict mode)
{
	FILE *f;
	int fd;
	int flags;

	/* Check for valid initial mode character */
	if (!strchr("rwa", *mode)) {
		errno = EINVAL;
		return 0;
	}

	/* Compute the flags to pass to open() */
	flags = __fmodeflags(mode);

	fd = sys_open(filename, flags, 0666);
	if (fd < 0) return 0;


	if (flags & O_CLOEXEC)
#if 0
		__syscall(SYS_fcntl, fd, F_SETFD, FD_CLOEXEC);
#endif
		sys_fcntl(fd, F_SETFD, (void *) FD_CLOEXEC);

	f = __fdopen(fd, mode);
	if (f) return f;

	sys_close(fd);
#if 0
	__syscall(SYS_close, fd);
#endif
	return 0;
}

#if 0
LFS64(fopen);
#endif
