#include <stdlib.h>
#if 0
#include <signal.h>
#endif

#include <syscall.h>

#if 0
#include <thread_impl.h>
#include <atomic.h>
#endif

_Noreturn void abort(void)
{
#if 0
	raise(SIGABRT);
	__block_all_sigs(0);
	a_crash();
	raise(SIGKILL);
#endif /* 0 */
	_Exit(127);

}
