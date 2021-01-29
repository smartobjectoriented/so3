
#include <sys/stat.h>
#include <syscall.h>

int stat(const char *pathname, struct stat *statbuf)
{
	return sys_stat(pathname, statbuf);
}
