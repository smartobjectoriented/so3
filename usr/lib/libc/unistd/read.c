#include <unistd.h>
#include <syscall.h>
#include <libc.h>

ssize_t read(int fd, void *buf, size_t count)
{
	return __syscall_ret(sys_read(fd, buf, count));
#if 0
	return syscall_cp(SYS_read, fd, buf, count);
#endif
}
