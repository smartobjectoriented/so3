#include <stdio_impl.h>
#include <syscall.h>

static int dummy(int fd)
{
	return fd;
}
weak_alias(dummy, __aio_close);

int __stdio_close(FILE *f)
{
#if 0
	return syscall(SYS_close, __aio_close(f->fd));
#endif
	return sys_close(__aio_close(f->fd));
}
