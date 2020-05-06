#include <time.h>
#include <sys/time.h>
#include "syscall.h"

int gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
	//struct timespec ts;
	if (!tv) return 0;

    sys_gettimeofday(tv);

    // the syscall return nanoseconds
    tv->tv_usec = tv->tv_usec / 1000ull;
	return 0;
}
