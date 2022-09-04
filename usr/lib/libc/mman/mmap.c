#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <syscall.h>
#include <libc.h>

#if 0 /* original musl implementation */
static void dummy(void) { }
weak_alias(dummy, __vm_wait);

#define UNIT SYSCALL_MMAP2_UNIT
#define OFF_MASK ((-0x2000ULL << (8*sizeof(syscall_arg_t)-1)) | (UNIT-1))
#endif

void *__mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{

	if (start || flags) {
		/* Issue warning for unsupported parameters. */
		printf("%s: start and flags parameters are not supported.\n", __func__);
	}

	if (!start)
		start = sbrk(0) + HEAP_SIZE;

	return sys_mmap((unsigned long) start, len, prot, fd, off);

#if 0 /* original musl implementation */
	if (off & OFF_MASK) {
		errno = EINVAL;
		return MAP_FAILED;
	}
	if (len >= PTRDIFF_MAX) {
		errno = ENOMEM;
		return MAP_FAILED;
	}
	if (flags & MAP_FIXED) {
		__vm_wait();
	}
#ifdef SYS_mmap2
	return (void *)syscall(SYS_mmap2, start, len, prot, flags, fd, off/UNIT);
#else
	return (void *)syscall(SYS_mmap, start, len, prot, flags, fd, off);
#endif
#endif
}

weak_alias(__mmap, mmap);

