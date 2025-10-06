#include <sys/socket.h>
#include "syscall.h"

int bind(int fd, const struct sockaddr *addr, socklen_t len)
{
	return __syscall_ret(sys_bind(fd, addr, len));
}
