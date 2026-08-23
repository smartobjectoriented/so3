/*
 * Copyright (c) 2020-2026 REDS Institute, HEIG-VD
 * Author: Julien Quartier <julien.quartier@heig-vd.ch>
 * Author: Daniel Rossier <daniel.rossier@heig-vd.ch>
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

/* ping: send ICMP echo requests to a host and report the replies, including
 * the round-trip times. */

#include <errno.h>
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
 * Human-readable reason for an ICMP message that is not an echo reply, or NULL
 * when the type is one we have nothing better to say about than its numbers.
 */
static const char *icmp_reason(unsigned char type, unsigned char code)
{
	switch (type) {
	case ICMP_DEST_UNREACH:
		switch (code) {
		case ICMP_NET_UNREACH:
			return "Destination Net Unreachable";
		case ICMP_HOST_UNREACH:
			return "Destination Host Unreachable";
		case ICMP_PROT_UNREACH:
			return "Destination Protocol Unreachable";
		case ICMP_PORT_UNREACH:
			return "Destination Port Unreachable";
		case ICMP_FRAG_NEEDED:
			return "Fragmentation needed but DF set";
		case ICMP_SR_FAILED:
			return "Source Route Failed";
		case ICMP_NET_ANO:
		case ICMP_HOST_ANO:
		case ICMP_PKT_FILTERED:
			return "Communication administratively prohibited";
		default:
			return "Destination Unreachable";
		}
	case ICMP_SOURCE_QUENCH:
		return "Source Quench";
	case ICMP_REDIRECT:
		return "Redirect";
	case ICMP_TIME_EXCEEDED:
		return (code == ICMP_EXC_TTL) ? "Time to live exceeded" : "Fragment reassembly time exceeded";
	case ICMP_PARAMETERPROB:
		return "Parameter problem";
	default:
		return NULL;
	}
}

/**
 * Compute the checksum
 * From https://www.geeksforgeeks.org/ping-in-c/
 */
unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2) {
		sum += *buf++;
	}
	if (len == 1) {
		sum += *(unsigned char *) buf;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

void show_help(void)
{
	printf("Usage: ping [-c count] [-i interval] [-t ttl]\n");
	printf("            [-W timeout] destination\n");
}

/*
 * Parse arg at pos arg.
 * Return the number of read values
 */
int parse_arg(int argc, int arg, char **argv)
{
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

parse_failed:
	printf("Argument parsing failed\n");
	show_help();
	exit(1);
}

void parse_args(int argc, char **argv)
{
	for (int i = 1; i < argc;) {
		i += parse_arg(argc - i, i, argv);
	}
}

int main(int argc, char **argv)
{
	int s, i = 0, msg_count = 0, msg_count_succeed = 0, attempt = 0;
	int len, hlen;
	unsigned int size = 0;
	float rtt = 0, rtt_total = 0.0, rtt_min = 1000000.0, rtt_max = 0.0;
	char ip[100];
	const char *reason;
	struct ping_pkt packet;
	char reply[sizeof(struct iphdr) + PING_PKT_LEN];
	struct iphdr *iph;
	struct icmphdr *icmph;
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
		printf("Cannot open the ICMP socket: %s\n", strerror(errno));
		return 1;
	}

	setsockopt(s, 0xfff, IP_TTL, (const char *) &ttl, sizeof(ttl));

	setsockopt(s, 0xfff, 0x1005, (const char *) &timeout, sizeof(struct timeval));
	setsockopt(s, 0xfff, 0x1006, (const char *) &timeout, sizeof(struct timeval));

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

		if (sendto(s, &packet, sizeof(packet), 0, (struct sockaddr *) &ping_addr, sizeof(ping_addr)) <= 0) {
			printf("Cannot send icmp_seq=%d: %s\n", msg_count, strerror(errno));
			continue;
		}

		size = sizeof(recv_addr);

		len = recvfrom(s, reply, sizeof(reply), 0, (struct sockaddr *) &recv_addr, &size);

		/* A host that never answers is the normal case, not a failure:
		 * SO_RCVTIMEO expires and lwIP reports it as EWOULDBLOCK (EAGAIN,
		 * the same value in musl). Anything else is a real error and says
		 * which one. */
		if (len <= 0) {
			if ((len < 0) && (errno != EAGAIN))
				printf("Cannot receive icmp_seq=%d: %s\n", msg_count, strerror(errno));
			else
				printf("Request timeout for icmp_seq=%d\n", msg_count);

			continue;
		}

		gettimeofday(&end, NULL);

		inet_ntop(AF_INET, &recv_addr.sin_addr, ip, sizeof(ip));

		rtt = end.tv_usec / 1000.0 + end.tv_sec * 1000 - (start.tv_usec / 1000.0 + start.tv_sec * 1000);

		/* A raw socket hands over the whole IP datagram, so the ICMP
		 * message starts after the IP header, whose length is given by
		 * the IHL field. */

		iph = (struct iphdr *) reply;
		hlen = iph->ihl * 4;

		if (len < hlen + (int) sizeof(struct icmphdr)) {
			printf("Error... Truncated reply of %d bytes\n", len);
			continue;
		}

		icmph = (struct icmphdr *) (reply + hlen);

		if (icmph->type == ICMP_ECHOREPLY && icmph->code == 0) {
			printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%f ms\n", len - hlen, ip, msg_count, iph->ttl, rtt);

			rtt_max = fmaxf(rtt_max, rtt);
			rtt_min = fminf(rtt_min, rtt);

			rtt_total += rtt;
			msg_count_succeed++;
		} else {
			/* Not an echo reply: an ICMP error about the request we
			 * just sent, reported by a router or by the stack of the
			 * destination itself. */

			reason = icmp_reason(icmph->type, icmph->code);

			if (reason != NULL)
				printf("From %s icmp_seq=%d %s\n", ip, msg_count, reason);
			else
				printf("From %s icmp_seq=%d ICMP type %d code %d\n", ip, msg_count, icmph->type, icmph->code);
		}
	}

	printf("\n--- %s ping statistics ---\n", destination);
	printf("%d packets transmitted, %d received, %f%% packet loss\n", msg_count, msg_count_succeed,
	       (1.0 - msg_count_succeed / (float) msg_count) * 100);

	if (msg_count_succeed > 0)
		printf("rtt min/avg/max = %f/%f/%f ms\n", rtt_min, rtt_total / msg_count_succeed, rtt_max);

	close(s);
	return 0;
}
