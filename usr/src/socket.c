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
    int s;
    struct ifreq ifr;
    struct ifreq ifr2;
    char *ip;

    s = socket(AF_INET, SOCK_STREAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "et0", IFNAMSIZ-1);

    ioctl(s, SIOCGIFADDR, &ifr);

    ip = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_ifru.ifru_addr)->sin_addr);
    printf("IP address: %s\n", ip);


    for(int i = 0; i < 10; i++){
        ifr2.ifr_ifru.ifru_ivalue = i;

        printf("%i: ", i);

        if(!ioctl(s, SIOCGIFNAME, &ifr2)){
            printf(" %s", ifr2.ifr_name);
        }
        printf("\n: ", i);


    }



    ioctl(s, SIOCGIFADDR, &ifr);

    ip = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_ifru.ifru_addr)->sin_addr);
    printf("IP address: %s\n", ip);

    close(s);

    return 0;
}
