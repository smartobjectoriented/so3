#include <stdio_impl.h>

off_t __stdio_seek(FILE *f, off_t off, int whence)
{

	off_t ret;

#ifdef SYS__llseek
	if (syscall(SYS__llseek, f->fd, off>>32, off, &ret, whence)<0)
		ret = -1;
#else
	ret = sys_lseek(f->fd, off, whence);
#if 0
	ret = syscall(SYS_lseek, f->fd, off, whence);
#endif
#endif

	return ret;

	return 0;
}
