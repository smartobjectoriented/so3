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
#include <arpa/inet.h>



int main(int argc, char **argv) {
    int s, i = 0, mtu = 0;
    struct ifreq ifr;
    char *ip, *mac;

    s = socket(AF_INET, SOCK_STREAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;

    for(i = 0; i < 256; i++){
        ifr.ifr_ifru.ifru_ivalue = i;

        // Check if if found
        if(ioctl(s, SIOCGIFNAME, &ifr)){
            continue;
        }

        printf("%s", ifr.ifr_name);


        if(ioctl(s, SIOCGIFHWADDR, &ifr) == 0){
            mac = ifr.ifr_ifru.ifru_hwaddr.sa_data;
            printf("\tHWaddr %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        }

        if(ioctl(s, SIOCGIFADDR, &ifr) == 0){
            ip = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_ifru.ifru_addr)->sin_addr);
            printf("\tinet addr:%s", ip);
        }

        if(ioctl(s, SIOCGIFBRDADDR, &ifr) == 0){
            ip = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_ifru.ifru_broadaddr)->sin_addr);
            printf("  Bcast:%s", ip);
        }

        if(ioctl(s, SIOCGIFNETMASK, &ifr) == 0){
            ip = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_ifru.ifru_netmask)->sin_addr);
            printf("  Mask:%s\n", ip);
        }

        if(ioctl(s, SIOCGIFMTU, &ifr) == 0){
            mtu = ifr.ifr_ifru.ifru_mtu;
            printf("\tMTU:%d\n", mtu);
        }

        printf("\n");
    }

    close(s);

    return 0;
}
