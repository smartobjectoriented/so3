#include <unistd.h>
#include <syscall.h>

int pipe(int fd[2])
{
	return sys_pipe(fd);
#if 0
#ifdef SYS_pipe
	return syscall(SYS_pipe, fd);
#else
	return syscall(SYS_pipe2, fd, 0);
#endif
#endif
}
