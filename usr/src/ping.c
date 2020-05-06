//
// Created by julien on 4/22/20.
//

#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>

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
    int s, i = 0, msg_count = 0;
    float elapsed_time = 0;
    //socklen_t addr_len;
    struct ping_pkt packet;
    struct sockaddr_in ping_addr/*, recv_addr*/;
    struct timeval tv_out, start, end;
    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;

    s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if(s < 0){
        printf("Impossible to obtain a socket file descriptor!!\n");
        return 0;
    }

    // TODO Set ttl

    // TODO set timeout

    setsockopt(s, 0xfff, 0x1005, (const char*)&tv_out, sizeof(struct timeval));
    setsockopt(s, 0xfff, 0x1006, (const char*)&tv_out, sizeof(struct timeval));


    while(i++ < 10){
        // TODO loop from here
        inet_pton(AF_INET, "10.0.2.2", &ping_addr.sin_addr);

        printf("%d: \n", i);

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

        gettimeofday(&start, NULL);

        if(sendto(s, &packet, sizeof(packet), 0, (struct sockaddr*)&ping_addr, sizeof(ping_addr)) <= 0){
            printf("Packet sending failed!!\n");
            continue;
        }

        if(recvfrom(s, &packet, sizeof(packet), 0, NULL, NULL) <= 0 && msg_count > 1){
            printf("Packet receive failed!!\n");
            continue;
        }

        gettimeofday(&end, NULL);


        elapsed_time = end.tv_usec / 1000.0 + end.tv_sec * 1000 - (start.tv_usec / 1000.0 + start.tv_sec * 1000);

        if(!(packet.hdr.type == 69 && packet.hdr.code == 0)){
            printf("Error... Packet received with ICMP type %d code %d\n", packet.hdr.type, packet.hdr.code);
        } else {
            printf("Packet Received ms %d\n", elapsed_time);
        }

        sleep(1);
    }

    return 0;



    /*end:

    close(s);
    return 0;*/
}
