#include <sys/socket.h>
#include <syscall.h>
#include <libc.h>

ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t alen)
{

	return sys_sendto(fd, buf, len, flags, addr, alen);

#if 0
	return socketcall_cp(sendto, fd, buf, len, flags, addr, alen);
#endif
}
