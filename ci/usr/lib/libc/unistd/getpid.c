#include <unistd.h>
#include <syscall.h>

pid_t getpid(void)
{
	return sys_getpid();
#if 0
	return __syscall(SYS_getpid);
#endif
}
