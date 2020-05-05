//
// Created by julien on 4/22/20.
//

#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>


#define PING_PKT_LEN 64


/**
 * Inspired by
 * https://www.geeksforgeeks.org/ping-in-c/
 */

struct ping_pkt
{
    struct icmphdr hdr;
    char msg[PING_PKT_LEN - sizeof(struct icmphdr)];
};

/**
 * Compute the checksum
 * From https://www.geeksforgeeks.org/ping-in-c/
 * @param b
 * @param len
 * @return
 */
unsigned short checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;

    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}


int main(int argc, char **argv) {
    int s, i, msg_count = 0;

    struct ping_pkt packet;
    struct sockaddr_in ping_addr;

    s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if(s < 0){
        printf("Impossible to obtain a socket file descriptor!!\n");
        return 0;
    }

    // TODO Set ttl

    // TODO set timeout


    // TODO loop from here
    inet_pton(AF_INET, "192.168.1.1", &ping_addr.sin_addr);

    ping_addr.sin_family = AF_INET;
    ping_addr.sin_port = 0; // ICMP -> no port

    memset(&packet, 0, sizeof(struct ping_pkt));

    packet.hdr.type = ICMP_ECHO;
    packet.hdr.un.echo.id = getpid();

    for(i = 0; i < sizeof(packet.msg) - 1; i++) {
        packet.msg[i] = i + '0';
    }
    packet.msg[i] = 0;
    packet.hdr.un.echo.sequence = msg_count++;
    packet.hdr.checksum = checksum(&packet, sizeof(packet));

    if(sendto(s, &packet, sizeof(packet), 0, (struct sockaddr*)&ping_addr, sizeof(ping_addr)) <= 0){
        printf("Packet sending failed!!\n");
        goto end;
    }






    end:

    close(s);
    return 0;
}
