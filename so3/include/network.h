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
int do_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int do_recv(int sockfd, void *mem, size_t len, int flags);
int do_recvfrom(int sockfd, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
int do_send(int sockfd, const void *dataptr, size_t size, int flags);
int do_sendto(int sockfd, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
int do_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

#endif //SO3_NETWORK_H
