#include <sys/socket.h>
#include "syscall.h"
#include "libc.h"

int connect(int fd, const struct sockaddr *addr, socklen_t len)
{
	return sys_connect(fd, addr, len);
}
