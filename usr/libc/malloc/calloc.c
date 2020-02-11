#include <stdlib.h>
#include <errno.h>

#include <malloc.h>

#if 0
void *__malloc0(size_t);
#endif

void *calloc(size_t m, size_t n)
{
	if (n && m > (size_t)-1/n) {
		errno = ENOMEM;
		return 0;
	}
#if 0
	return __malloc0(n * m);
#endif
	return malloc(n * m);
}
