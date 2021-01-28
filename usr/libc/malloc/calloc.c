#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <malloc.h>

#if 0
void *__malloc0(size_t);
#endif

void *calloc(size_t m, size_t n)
{
	void *__area;

	if (n && m > (size_t)-1/n) {
		errno = ENOMEM;
		return NULL;
	}
#if 0
	return __malloc0(n * m);
#endif
	__area = malloc(n * m);

	/* According to POSIX, the memory is set to 0. */
	if (!__area) {
		errno = ENOMEM;
		return NULL;
	}

	memset(__area, 0, n*m);

	return __area;
}
