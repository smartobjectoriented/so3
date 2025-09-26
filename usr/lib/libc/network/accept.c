#include <sys/socket.h>
#include "syscall.h"
#include "libc.h"

int accept(int fd, struct sockaddr *restrict addr, socklen_t *restrict len)
{
	return __syscall_ret(sys_accept(fd, addr, len));
}
