#include <stdlib.h>
#include <syscall.h>

_Noreturn void _Exit(int ec)
{
	sys_exit(ec);

#if 0
	__syscall(SYS_exit_group, ec);
	for (;;) __syscall(SYS_exit, ec);
#endif /* 0 */
}
