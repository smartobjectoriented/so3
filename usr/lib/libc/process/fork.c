#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "syscall.h"
#include "libc.h"
#include "pthread_impl.h"

static void dummy(int x)
{
}

weak_alias(dummy, __fork_handler);

pid_t fork(void)
{
	pid_t ret;
#if 0 /* Not used at the moment in so3 */
	sigset_t set;

	__fork_handler(-1);
	__block_all_sigs(&set);
#endif /* 0 */

	ret = sys_fork();
#if 0
#ifdef SYS_fork
	ret = syscall(SYS_fork);
#else
	ret = syscall(SYS_clone, SIGCHLD, 0);
#endif
#endif
#if 0
	if (!ret) {
		pthread_t self = __pthread_self();
		self->tid = __syscall(SYS_gettid);
		self->robust_list.off = 0;
		self->robust_list.pending = 0;
		libc.threads_minus_1 = 0;
	}
	__restore_sigs(&set);
	__fork_handler(!ret);
#endif
	return ret;
}
