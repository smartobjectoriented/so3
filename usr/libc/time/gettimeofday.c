#include <time.h>
#include <sys/time.h>
#include "syscall.h"
#include <errno.h>

int gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
        if (!tv) {
                errno = EFAULT;
                return -1;
        }

        return sys_gettimeofday(tv);
}
