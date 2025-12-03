
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <sys/time.h>

int main(int argc, char *argv[])
{
	struct timeval tv;

	while (true) {
		gettimeofday(&tv, NULL);

		printf("# time(s) : %lu  time(us) : %lu\n", tv.tv_sec, tv.tv_usec);
	}
}
