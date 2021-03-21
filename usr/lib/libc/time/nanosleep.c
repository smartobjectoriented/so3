#include <time.h>
#include <syscall.h>
#include <libc.h>

int nanosleep(const struct timespec *req, struct timespec *rem)
{
	return sys_nanosleep(req, rem);
}
