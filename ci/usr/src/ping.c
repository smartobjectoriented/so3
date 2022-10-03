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

#define PING_PKT_LEN 64

int count = 10;
int ttl = 64;
int timeout_s = 2;
float interval_s = 1.0f;
char *destination = NULL;

/**
 * Inspired by
 * https://www.geeksforgeeks.org/ping-in-c/
 */
struct ping_pkt {
	struct icmphdr hdr;
	char msg[PING_PKT_LEN - sizeof(struct icmphdr)];
};

/**
 * Compute the checksum
 * From https://www.geeksforgeeks.org/ping-in-c/
 */
unsigned short checksum(void *b, int len) {
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2) {
		sum += *buf++;
	}
	if (len == 1) {
		sum += *(unsigned char*) buf;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

void show_help(void) {
	printf("Usage: ping [-c count] [-i interval] [-t ttl]\n");
	printf("            [-W timeout] destination\n");
}

/*
 * Parse arg at pos arg.
 * Return the number of read values
 */
int parse_arg(int argc, int arg, char **argv) {
	size_t len = strlen(argv[arg]);
	int tmp = 0;
	float tmp_f = 0;

	if (len == 2) {
		if (argv[arg][0] != '-')
			goto parse_failed;

		switch (argv[arg][1]) {
		case 'h':
			show_help();
			exit(0);
		case 'c':
			if (argc < 2)
				goto parse_failed;

			tmp = atoi(argv[arg + 1]);
			if (tmp <= 0)
				goto parse_failed;

			count = tmp;
			return 2;
		case 't':
			if (argc < 2)
				goto parse_failed;

			tmp = atoi(argv[arg + 1]);
			if (tmp <= 0)
				goto parse_failed;

			timeout_s = tmp;
			return 2;
		case 'i':
			if (argc < 2)
				goto parse_failed;

			tmp_f = atof(argv[arg + 1]);
			if (tmp_f <= 0.0f)
				goto parse_failed;

			interval_s = tmp_f;
			return 2;
		case 'W':
			if (argc < 2)
				goto parse_failed;

			tmp = atoi(argv[arg + 1]);
			if (tmp <= 0)
				goto parse_failed;

			timeout_s = tmp;
			return 2;
		default:
			goto parse_failed;
		}
	} else {
		destination = argv[arg];
	}

	return 1;

	parse_failed: printf("Argument parsing failed\n");
	show_help();
	exit(1);
}

void parse_args(int argc, char **argv) {
	for (int i = 1; i < argc;) {
		i += parse_arg(argc - i, i, argv);
	}
}

int main(int argc, char **argv) {
	int s, i = 0, msg_count = 0, msg_count_succeed = 0, attempt = 0;
	unsigned int size = 0;
	float rtt = 0, rtt_total, rtt_min = 1000000.0, rtt_max = 0.0;
	char ip[100];
	struct ping_pkt packet;
	struct sockaddr_in ping_addr, recv_addr;
	struct timeval timeout, start, end;

	timeout.tv_sec = timeout_s;
	timeout.tv_usec = 0;

	parse_args(argc, argv);

	if (destination == NULL) {
		printf("A destination is required\n");
		show_help();
		return 1;
	}

	s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (s < 0) {
		printf("Impossible to obtain a socket file descriptor!!\n");
		return 1;
	}

	setsockopt(s, 0xfff, IP_TTL, (const char*) &ttl, sizeof(ttl));

	setsockopt(s, 0xfff, 0x1005, (const char*) &timeout,
			sizeof(struct timeval));
	setsockopt(s, 0xfff, 0x1006, (const char*) &timeout,
			sizeof(struct timeval));

	while (attempt++ < count) {
		usleep((unsigned) (interval_s * 1000000u));

		inet_pton(AF_INET, destination, &ping_addr.sin_addr);

		ping_addr.sin_family = AF_INET;
		ping_addr.sin_port = 0; /* ICMP -> no port */

		memset(&packet, 0, sizeof(struct ping_pkt));

		packet.hdr.type = ICMP_ECHO;
		packet.hdr.un.echo.id = getpid();

		for (i = 0; i < sizeof(packet.msg) - 1; i++) {
			packet.msg[i] = i + '0';
		}
		packet.msg[i] = 0;
		packet.hdr.un.echo.sequence = msg_count++;
		packet.hdr.checksum = checksum(&packet, sizeof(packet));

		gettimeofday(&start, NULL);

		if (sendto(s, &packet, sizeof(packet), 0,
				(struct sockaddr*) &ping_addr,
				sizeof(ping_addr)) <= 0) {
			printf("Packet sending failed!!\n");
			continue;
		}

		size = sizeof(recv_addr);

		if (recvfrom(s, &packet, sizeof(packet), 0,
				(struct sockaddr*) &recv_addr, &size) <= 0
				&& msg_count > 1) {
			printf("Packet receive failed!!\n");
			continue;
		}

		gettimeofday(&end, NULL);

		inet_ntop(AF_INET, &recv_addr.sin_addr, ip, sizeof(ip));

		rtt = end.tv_usec / 1000.0 + end.tv_sec * 1000 - (start.tv_usec / 1000.0 + start.tv_sec	* 1000);

		if (!(packet.hdr.type == 69 && packet.hdr.code == 0)) {
			printf(
					"Error... Packet received with ICMP type %d code %d\n",
					packet.hdr.type, packet.hdr.code);
		} else {
			printf(
					"%d bytes from %s: icmp_seq=%d ttl=%d time=%Lf ms\n",
					PING_PKT_LEN, ip, msg_count, ttl, rtt);

			rtt_max = fmaxf(rtt_max, rtt);
			rtt_min = fminf(rtt_min, rtt);

			rtt_total += rtt;
			msg_count_succeed++;
		}
	}

	printf("\n--- %s ping statistics ---\n", destination);
	printf("%d packets transmitted, %d received, %d%% packet loss\n",
			msg_count, msg_count_succeed,
			(1.0 - msg_count_succeed / (float) msg_count) * 100);

	if (msg_count_succeed > 0)
		printf("rtt min/avg/max = %Lf/%Lf/%Lf ms\n", rtt_min,
				rtt_total / msg_count_succeed, rtt_max);

	return 0;

	/*end:

	 close(s);
	 return 0;*/
}
