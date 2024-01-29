#include "prof_test_p.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
	printf("Testing profiler\n");
	testFunc1();
	testFunc2(3.5);
	return 0;
}
