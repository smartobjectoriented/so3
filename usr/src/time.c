
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <sys/time.h>

int main(int argc, char *argv[])
{
	time_t t;
	struct timeval tv;

	while (true) {
		gettimeofday(&tv, NULL);

		time(&t);

		printf("# time(s) : %lu  time(us) : %lu\n", t, tv.tv_usec);
	}
}
