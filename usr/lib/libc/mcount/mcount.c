#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/**
 * @brief Timestamps the entry time of the function call identified by its return address
 * @param fn_lr Return address of the profiled function call
 * 
*/
void mcount(uintptr_t fn_lr)
{
	printf("Entered func with identifier %lx\n", fn_lr);
	//TODO Get timestamp
	//TODO Write timestamp with identifier
}

/**
 * @brief Timestamps the exit time of the function call identified by its return address
 * @param fn_lr Return address of the profiled function call
*/
void mcount_exit(uintptr_t fn_lr)
{
	printf("Exited func with identifier %lx\n", fn_lr);
	//TODO Get timestamp
	//TODO Write timestamp with identifier
}
