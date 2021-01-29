/*
 * Copyright (C) 2020 Julien Quartier <julien.quartier@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#define PORT 5000

char buff[1024];
float send_burst(int s, int kB) {
	int sent = 0, written, error = 0;
	float rtt_ms = 0;
	struct timeval start, end;

	printf("\nStartig burst of %d kB\n", kB);

	gettimeofday(&start, NULL);

	while (sent++ < kB) {
		memset(buff, sent, sizeof(buff) - 1);
		buff[1023] = 0;
		if ((written = write(s, buff, sizeof(buff))) <= 0) {
			printf("Write error \n");
			error++;
			continue;
		}
	}
	gettimeofday(&end, NULL);
	rtt_ms = end.tv_usec / 1000.0 + end.tv_sec * 1000 - (start.tv_usec / 1000.0 + start.tv_sec * 1000);

	printf("%d kB in %Lf ms\n", kB, rtt_ms);
	printf("%f mb/s\n", (kB * 8.0 / 1024) / (rtt_ms / 1000.0));

	return rtt_ms;
}

int main(int argc, char **argv) {

	int s = 0, read_len, max_kB = 1000, current_kB = 1, kB_increment = 10;
	float rtt_ms = 0, max_rtt_ms = 60 * 1000;
	struct sockaddr_in srv_addr;
	struct timeval timeout;

	if (argc > 2) {
		max_kB = atoi(argv[2]);
	}

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	memset(buff, 0, sizeof(buff));
	memset(&srv_addr, 0, sizeof(srv_addr));

	s = socket(AF_INET, SOCK_STREAM, 0);

	setsockopt(s, 0xfff, 0x1005, (const char*) &timeout,
			sizeof(struct timeval));
	setsockopt(s, 0xfff, 0x1006, (const char*) &timeout,
			sizeof(struct timeval));

	if (s < 0) {
		printf("Impossible to obtain a socket file descriptor!!\n");
		return 1;
	}

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, *(argv + 1), &srv_addr.sin_addr) <= 0) {
		printf("\n inet_pton error occured\n");
		goto com_error;
	}

	if (connect(s, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0) {
		printf("\n Error : Connect Failed %d\n", errno);
		goto com_error;
	}

	if ((read_len = read(s, buff, sizeof(buff) - 1)) > 0) {
		buff[read_len] = 0;
		//printf("The server said: %s", buff);
	}

	if (read_len < 0) {
		printf("Impossible to read the message \n");
		goto com_error;
	}

	do {
		rtt_ms = send_burst(s, current_kB);
	} while (rtt_ms * kB_increment < max_rtt_ms && (current_kB *= kB_increment) <= max_kB);

	if (rtt_ms * kB_increment > max_rtt_ms)
		printf("Aborted as next burst estimated rtt(%Lf ms) exeeded the max rtt of %Lf ms \n", rtt_ms, max_rtt_ms);

	close(s);

	return 0;

com_error:
	close(s);

	return 1;
}

