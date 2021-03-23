#include <stdio.h>

int putchar(int c)
{
	int ret;

	ret = fputc(c, stdout);

	/* Flush immediately */
	fflush(stdout);

	return ret;
}
