#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

/**
 * @brief Timestamps the entry time of the function call identified by its return address
 * @param fn_lr Return address of the profiled function call
 * 
*/
void mcount(uintptr_t fn_lr)
{
	struct timespec ts;
	// Get timestamp
	clock_gettime(CLOCK_REALTIME, &ts);
	// Write timestamp with identifier
	printf("prof > %lx: %llu.%llu\n", fn_lr, ts.tv_sec, ts.tv_nsec);
}

/**
 * @brief Timestamps the exit time of the function call identified by its return address
 * @param fn_lr Return address of the profiled function call
*/
void mcount_exit(uintptr_t fn_lr)
{	
	struct timespec ts;
	// Get timestamp
	clock_gettime(CLOCK_REALTIME, &ts);
	// Write timestamp with identifier
	printf("prof < %lx: %llu.%llu\n", fn_lr, ts.tv_sec, ts.tv_nsec);
}
