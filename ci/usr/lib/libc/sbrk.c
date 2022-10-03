#include <stdint.h>
#include <errno.h>
#include <syscall.h>
#include <unistd.h>

void *sbrk(intptr_t inc)
{
	return sys_sbrk(0);
#if 0
	if (inc) return (void *)__syscall_ret(-ENOMEM);
	return (void *)__syscall(SYS_brk, 0);
#endif
}


