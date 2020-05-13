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


void sig_handler(int dummy) {
        printf("end");
        exit(0);
}

int server()
{
        int s, connfd, read_len;
        struct sockaddr_in srv_addr, client_addr;
        char buff[1025];

        memset(buff, 0, sizeof(buff));
        memset(&client_addr, 0, sizeof(client_addr));
        memset(&srv_addr, 0, sizeof(srv_addr));

        s = socket(AF_INET, SOCK_STREAM, 0);

        if (s < 0) {
                printf("Impossible to obtain a socket file descriptor!!\n");
                return 1;
        }

        srv_addr.sin_family = AF_INET;
        srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        srv_addr.sin_port = htons(5000);

        if(bind(s, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0){
                printf("Impossible to bind\n");
                return -1;
        }

        if(listen(s, 10) < 0){
                printf("Impossible to listen\n");
                return -1;
        }

        while (1) {
                printf("\nWaiting for clients...\n");

                connfd = accept(s, NULL, NULL);
                if(connfd < 0){
                        printf("Error on accept\n");
                        continue;
                }

                printf("New client connected\n");

                snprintf(buff, sizeof(buff), "Hello world %d\n", s);
                write(connfd, buff, strlen(buff));

                if ((read_len = read(connfd, buff, sizeof(buff) - 1)) > 0) {
                        buff[read_len] = 0;
                        printf("The client said: %s", buff);
                }


                if(read_len < 0){
                        printf("Impossible to read the message \n");
                        goto end_client;
                }

         end_client:
                close(connfd);
                usleep(1000);
        }

        return 0;
}

int client(const char *ip)
{
        int s = 0, read_len = 0;
        char buff[1025];
        struct sockaddr_in srv_addr;

        memset(buff, 0, sizeof(buff));
        memset(&srv_addr, 0, sizeof(srv_addr));


        s = socket(AF_INET, SOCK_STREAM, 0);

        if (s < 0) {
                printf("Impossible to obtain a socket file descriptor!!\n");
                return 1;
        }

        srv_addr.sin_family = AF_INET;
        srv_addr.sin_port = htons(5000);

        if (inet_pton(AF_INET, ip, &srv_addr.sin_addr) <= 0) {
                printf("\n inet_pton error occured\n");
                goto com_error;
        }

        if (connect(s, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0) {
                printf("\n Error : Connect Failed %d\n", errno);
                goto com_error;
        }

        if ((read_len = read(s, buff, sizeof(buff) - 1)) > 0) {
                buff[read_len] = 0;
                printf("The server said: %s", buff);
        }

        if (read_len < 0) {
                printf("Impossible to read the message \n");
                goto com_error;
        }

        snprintf(buff, sizeof(buff), "I'm not the world!\n");
        write(s, buff, strlen(buff));

        close(s);
        return 0;

        com_error:
        close(s);
        return 1;
}

int main(int argc, char **argv)
{

        signal(SIGINT, sig_handler);


        if (argc == 1)
                server();
        else if (argc == 2)
                client(argv[1]);


        return 0;
}
