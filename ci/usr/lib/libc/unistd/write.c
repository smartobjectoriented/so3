#include <unistd.h>
#include "syscall.h"
#include "libc.h"

ssize_t write(int fd, const void *buf, size_t count)
{
	return sys_write(fd, buf, count);

#if 0
	return syscall_cp(SYS_write, fd, buf, count);
#endif
}
