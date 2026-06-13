
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <sys/time.h>
#include <inttypes.h>

int main(int argc, char *argv[])
{
	struct timeval tv;

	while (true) {
		gettimeofday(&tv, NULL);

		printf("# time(s) : %" PRIu64 "  time(us) : %" PRIu64 "\n", tv.tv_sec, tv.tv_usec);
	}
}
