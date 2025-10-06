#include <unistd.h>
#include <syscall.h>

int dup(int fd)
{
	return __syscall_ret(sys_dup(fd));
}
