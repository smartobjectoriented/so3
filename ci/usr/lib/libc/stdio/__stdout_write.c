#include <stdio_impl.h>
#include <sys/ioctl.h>
#include <syscall.h>


size_t __stdout_write(FILE *f, const unsigned char *buf, size_t len)
{
#if 0
	struct winsize wsz;
#endif

	f->write = __stdio_write;

#if 0
	if (!(f->flags & F_SVB) && __syscall(SYS_ioctl, f->fd, TIOCGWINSZ, &wsz))
		f->lbf = -1;
#endif

	return __stdio_write(f, buf, len);
}
