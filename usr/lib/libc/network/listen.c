#include <sys/socket.h>
#include "syscall.h"

int listen(int fd, int backlog)
{
	return sys_listen(fd, backlog);
}
