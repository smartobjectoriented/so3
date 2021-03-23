#include <signal.h>
#include <errno.h>
#include <string.h>
#include <syscall.h>
#include <libc.h>
#include <atomic.h>
#include <ksigaction.h>

#if 0 /* unused in SO3 */
static int unmask_done;
#endif

static unsigned long handler_set[_NSIG/(8*sizeof(long))];

void __get_handler_set(sigset_t *set)
{
	memcpy(set, handler_set, sizeof handler_set);
}

typedef void (*sigaction_handler_t)(int);

/* SO3 */
/*
 * Trampoline function to call the handler
 */
void __restore(int signum, void *handler) {
	sigaction_handler_t __handler;

	/* First call the handler, then back to the kernel */
	__handler = (sigaction_handler_t) handler;
	__handler(signum);

	sys_sigreturn();
}

void __restore_rt(int signum, void *handler) {
	__restore(signum, handler);
}

int __libc_sigaction(int sig, const struct sigaction *restrict sa, struct sigaction *restrict old)
{
	struct k_sigaction ksa, ksa_old;
	if (sa) {
		if ((uintptr_t)sa->sa_handler > 1UL) {
			a_or_l(handler_set+(sig-1)/(8*sizeof(long)),
				1UL<<(sig-1)%(8*sizeof(long)));

			/* If pthread_create has not yet been called,
			 * implementation-internal signals might not
			 * yet have been unblocked. They must be
			 * unblocked before any signal handler is
			 * installed, so that an application cannot
			 * receive an illegal sigset_t (with them
			 * blocked) as part of the ucontext_t passed
			 * to the signal handler. */
#if 0 /* Not set available on SO3 */
			if (!libc.threaded && !unmask_done) {


				__syscall(SYS_rt_sigprocmask, SIG_UNBLOCK,
					SIGPT_SET, 0, _NSIG/8);

				unmask_done = 1;
			}
#endif /* 0 */
		}

		ksa.handler = sa->sa_handler;
		ksa.flags = sa->sa_flags | SA_RESTORER;
		ksa.restorer = (sa->sa_flags & SA_SIGINFO) ? __restore_rt : __restore;
		memcpy(&ksa.mask, &sa->sa_mask, sizeof ksa.mask);
	}

	/* SO3 */

	sys_sigaction(sig, sa ? &ksa : 0, old ? &ksa_old : 0);

	if (old) {
		old->sa_handler = ksa_old.handler;
		old->sa_flags = ksa_old.flags;
		memcpy(&old->sa_mask, &ksa_old.mask, sizeof ksa_old.mask);
	}
	return 0;
}

int __sigaction(int sig, const struct sigaction *restrict sa, struct sigaction *restrict old)
{
	if (sig-32U < 3 || sig-1U >= _NSIG-1) {
		errno = EINVAL;
		return -1;
	}
	return __libc_sigaction(sig, sa, old);
}

weak_alias(__sigaction, sigaction);
