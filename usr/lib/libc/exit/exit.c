#include <stdlib.h>
#include <stdint.h>
#include <libc.h>

static void dummy()
{
}

/* atexit.c and __stdio_exit.c override these. the latter is linked
 * as a consequence of linking either __toread.c or __towrite.c. */
weak_alias(dummy, __funcs_on_exit);
weak_alias(dummy, __stdio_exit);
weak_alias(dummy, _fini);

__attribute__((__weak__, __visibility__("hidden")))
extern void (*const __fini_array_start)(void), (*const __fini_array_end)(void);

static void libc_exit_fini(void)
{
	uintptr_t a = (uintptr_t)&__fini_array_end;
	for (; a>(uintptr_t)&__fini_array_start; a-=sizeof(void(*)()))
		(*(void (**)())(a-sizeof(void(*)())))();
	_fini();
}

weak_alias(libc_exit_fini, __libc_exit_fini);

_Noreturn void exit(int code)
{
	#warning exit() issue
	/* 
	 * Currently, the following code leads to data abort fault randomly. The effect
	 * is visible on RPi4 (64-bit) more rarely on virt64
	 */
#if 0
	__funcs_on_exit();
	__libc_exit_fini();	
	__stdio_exit();
#endif /* 0 */

	_Exit(code);
}
