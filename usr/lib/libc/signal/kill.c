#include <signal.h>
#include <syscall.h>

int kill(pid_t pid, int sig)
{
	return sys_kill(pid, sig);

#if 0 /* SO3 */
	return syscall(SYS_kill, pid, sig);
#endif
}
