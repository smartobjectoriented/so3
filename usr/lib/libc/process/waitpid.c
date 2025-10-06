#include <sys/wait.h>
#include <syscall.h>

#include <libc.h>

pid_t waitpid(pid_t pid, int *status, int options)
{
	return __syscall_ret(sys_waitpid(pid, status, options));
#if 0
	return syscall_cp(SYS_wait4, pid, status, options, 0);
#endif
}
