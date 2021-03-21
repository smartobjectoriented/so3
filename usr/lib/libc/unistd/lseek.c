#include <unistd.h>
#include <syscall.h>
#include <libc.h>

off_t lseek(int fd, off_t offset, int whence)
{
#ifdef SYS__llseek
	off_t result;
	return syscall(SYS__llseek, fd, offset>>32, offset, &result, whence) ? -1 : result;
#else
	return sys_lseek(fd, offset, whence);
#if 0
	return syscall(SYS_lseek, fd, offset, whence);
#endif

#endif
}

LFS64(lseek);
