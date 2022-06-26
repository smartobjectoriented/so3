#include <time.h>
#include <sys/time.h>
#include <syscall.h>
#include <errno.h>

int gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
	struct timespec ts;
	if (!tv) return 0;

	clock_gettime(CLOCK_REALTIME, &ts);

	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / (time_t) 1000;
	return 0;
}
