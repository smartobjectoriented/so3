
#include <sys/stat.h>
#include <syscall.h>

int stat(const char *pathname, struct stat *statbuf)
{
	return __syscall_ret(sys_stat(pathname, statbuf));
}
