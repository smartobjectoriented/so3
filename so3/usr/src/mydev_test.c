#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

int main(void)
{
	char write_buffer[20];
	char read_buffer[20];

	int fd = open("/dev/mydev", O_RDWR);

	sprintf(write_buffer, "Hello My Dev!");
	write(fd, write_buffer, strlen(write_buffer) + 1);

	read(fd, read_buffer, sizeof(read_buffer));

	printf("Wrote: %s - Read %s\n", write_buffer, read_buffer);
	assert(!strcmp(write_buffer, read_buffer));
	return 0;
}
