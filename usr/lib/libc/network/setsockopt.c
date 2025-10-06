#include <sys/socket.h>
#include "syscall.h"

int setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	return __syscall_ret(sys_setsockopt(fd, level, optname, optval, optlen));
}
