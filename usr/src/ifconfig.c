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


enum addr_type{
        UNDEFINED,
        HOST,
        NETMASK,
        BROADCAST,
};

void show_help()
{
        printf("Usage: ifconfig TODO...\n");
}

void set_mtu(struct ifreq *ifr, int mtu)
{
        int s = socket(AF_INET, SOCK_STREAM, 0);

        ifr->ifr_ifru.ifru_mtu = mtu;

        if(ioctl(s, SIOCSIFMTU, ifr)){
                printf("Error while configuring mtu. Errno: %d", errno);
                close(s);
                exit(0);
        }

        close(s);
}

void set_up(char* name)
{

}

void set_down(char* name)
{

}

void set_ip(struct ifreq *ifr, enum addr_type type)
{
        int req, s;

        switch(type){
        case HOST:
                req = SIOCSIFADDR;
                break;
        case NETMASK:


                req = SIOCSIFNETMASK;
                break;
        case BROADCAST:
                req = SIOCSIFBRDADDR;
                break;
        default:
                return;
        }

        s = socket(AF_INET, SOCK_STREAM, 0);

        if(ioctl(s, req, ifr)){
                printf("Error while configuring address. Errno: %d\n", errno);
                close(s);
                exit(0);
        }

        close(s);
}


void show_if(char* name)
{
        struct ifreq ifr;
        int mtu, s;
        char *ip, *mac;

        s = socket(AF_INET, SOCK_STREAM, 0);
        ifr.ifr_addr.sa_family = AF_INET;

        strncpy(ifr.ifr_name, name, 15);
        ifr.ifr_name[15] = 0;

        if(ioctl(s, SIOCGIFINDEX, &ifr) != 0) {
                printf("Interface '%s' not found\n", ifr.ifr_name);
                close(s);
                return;
        }

        printf("%s", ifr.ifr_name);


        if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0) {
                mac = ifr.ifr_ifru.ifru_hwaddr.sa_data;
                printf("\tHWaddr %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }

        if (ioctl(s, SIOCGIFADDR, &ifr) == 0) {
                ip = inet_ntoa(((struct sockaddr_in *) &ifr.ifr_ifru.ifru_addr)->sin_addr);
                printf("\tinet addr:%s", ip);
        }

        if (ioctl(s, SIOCGIFBRDADDR, &ifr) == 0) {
                ip = inet_ntoa(((struct sockaddr_in *) &ifr.ifr_ifru.ifru_broadaddr)->sin_addr);
                printf("  Bcast:%s", ip);
        }

        if (ioctl(s, SIOCGIFNETMASK, &ifr) == 0) {
                ip = inet_ntoa(((struct sockaddr_in *) &ifr.ifr_ifru.ifru_netmask)->sin_addr);
                printf("  Mask:%s\n", ip);
        }

        if (ioctl(s, SIOCGIFMTU, &ifr) == 0) {
                mtu = ifr.ifr_ifru.ifru_mtu;
                printf("\tMTU:%d\n", mtu);
        }

        printf("\n");

        close(s);
}


void scan_ifs(){
        int s, i = 0;
        struct ifreq ifr;

        s = socket(AF_INET, SOCK_STREAM, 0);

        ifr.ifr_addr.sa_family = AF_INET;

        for (i = 0; i < 256; i++) {
                ifr.ifr_ifru.ifru_ivalue = i;

                /* Check if if found */
                if (!ioctl(s, SIOCGIFNAME, &ifr))
                        show_if(ifr.ifr_name);
        }

        close(s);

}

enum addr_type get_addr_type(char* addr_type){
        if(!strcmp(addr_type, "netmask"))
                return NETMASK;
        else if(!strcmp(addr_type, "broadcast"))
                return BROADCAST;


        return UNDEFINED;
}

void parse_args(int argc, char **argv)
{
        //int val = 0;
        int tmp, addr_type = 0;
        //size_t len;
        struct sockaddr_in *addr;
        struct ifreq ifr;

        addr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_addr;

        addr->sin_family = AF_INET;


        argc--;
        argv++;

        if(argc == 0) {
                scan_ifs();
                return;
        }

        while(argc && strlen(*argv) == 2 && *argv[0] == '-'){
                switch (*argv[1]){
                case 'h':
                        show_help();
                        exit(0);
                default:
                        goto parse_failed;
                }

                argc--;
                argv++;
        }

        /* only one parameter display the interface */
        if(argc == 1){
                show_if(*argv);
                return;
        }

        /* more parameters to come, prepare if request with name */
        strncpy(ifr.ifr_name, *argv, IF_NAMESIZE - 1);
        ifr.ifr_name[IF_NAMESIZE - 1] = 0;

        argc--;
        argv++;

        while(argc && *argv != NULL){
                if (inet_pton(AF_INET, *argv, &addr->sin_addr)){
                        set_ip(&ifr, HOST);
                        argc--;
                        argv++;
                        continue;
                }

                if((addr_type = get_addr_type(*argv)) != UNDEFINED){
                        argc--;
                        argv++;

                        if(argv != NULL && inet_pton(AF_INET, *argv, &addr->sin_addr)){
                                set_ip(&ifr, addr_type);
                                argc--;
                                argv++;
                                continue;
                        }

                        printf("'%s' must be followed by a valid ip\n", *argv);
                        goto parse_failed;
                }

                if(!strcmp("mtu", *argv)) {
                        argc--;
                        argv++;

                        if(*argv == NULL)
                                goto parse_failed;

                        tmp = atoi(*argv);
                        if(tmp > 0) {
                                set_mtu(&ifr, tmp);
                                argc--;
                                argv++;
                                continue;
                        }

                        goto parse_failed;
                }

                printf("Argument '%s' unknown\n", *argv);
                goto parse_failed;
        }


#if 0
        if (len > 2) {
                /* only one parameter display the interface */
                if(argc == 1){
                        show_if(argv[arg]);
                        return 1;
                }


                if(argc > 2){
                        if(!inet_pton(AF_INET, argv[arg + 1], &addr.sin_addr)){
                                set_ip(argv[arg], &addr);
                        }

                        if(strcmp("mtu", argv[arg + 1])){
                                val = atoi(argv[arg + 1]);
                                if(val <= 0)
                                        goto parse_failed;

                                set_mtu(argv[arg], val);

                                return 2;
                        }
                }
        }
#endif
        return;

        parse_failed:
        printf("Argument parsing failed\n");
        show_help();
        exit(1);
}


int main(int argc, char **argv)
{
        parse_args(argc, argv);

        return 0;
}
