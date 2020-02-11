#include <unistd.h>
#include <syscall.h>

int dup(int fd)
{
	return sys_dup(fd);
}
