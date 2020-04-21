//
// Created by julien on 3/25/20.
//

#ifndef SO3_NETWORK_H
#define SO3_NETWORK_H

#include <vfs.h>

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <device/network.h>

void network_init(void);

int get_lwip_fd(int gfd);

int do_socket(int domain, int type, int protocol);
int do_connect(int sockfd, const struct sockaddr *name, socklen_t namelen);
int do_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int do_listen(int sockfd, int backlog);




#endif //SO3_NETWORK_H
