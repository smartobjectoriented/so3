#include <sys/socket.h>
#include "syscall.h"

int listen(int fd, int backlog)
{
	return __syscall_ret(sys_listen(fd, backlog));
}
