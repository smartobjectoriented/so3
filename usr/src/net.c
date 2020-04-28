
#include <sys/socket.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUFSIZE 1024

char buf[BUFSIZE];

int main(int argc, char **argv) {
	int fd;

	if (argc != 2) {
		printf("Usage: net <netif>\n");
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		printf("Unable to open %s\n", argv[1]);
		return 1;
	}

	close(fd);

	return 0;
}
