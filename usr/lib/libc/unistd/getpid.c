#include <unistd.h>
#include <syscall.h>

pid_t getpid(void)
{
	return __syscall_ret(sys_getpid());
#if 0
	return __syscall(SYS_getpid);
#endif
}
