
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include "__dirent.h"
#include <syscall.h>

#include <stdio.h>
DIR *opendir(const char *name)
{
	int fd;
	DIR *dir;

	if ((fd = open(name, O_RDONLY|O_DIRECTORY|O_CLOEXEC)) < 0)
		return 0;
	if (!(dir = calloc(1, sizeof *dir))) {
		sys_close(fd);
#if 0
		__syscall(SYS_close, fd);
#endif
		return 0;
	}

	dir->fd = fd;

	return dir;
}
