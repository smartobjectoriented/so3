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

#ifndef NET_H
#define NET_H

#include <vfs.h>
#include <syscall.h>

#include <net/lwip/sockets.h>

#include <device/net.h>

#define SIOCADDRT 0x890B
#define SIOCDELRT 0x890C
#define SIOCRTMSG 0x890D

#define SIOCGIFNAME 0x8910
#define SIOCSIFLINK 0x8911
#define SIOCGIFCONF 0x8912
#define SIOCGIFFLAGS 0x8913
#define SIOCSIFFLAGS 0x8914
#define SIOCGIFADDR 0x8915
#define SIOCSIFADDR 0x8916
#define SIOCGIFDSTADDR 0x8917
#define SIOCSIFDSTADDR 0x8918
#define SIOCGIFBRDADDR 0x8919
#define SIOCSIFBRDADDR 0x891a
#define SIOCGIFNETMASK 0x891b
#define SIOCSIFNETMASK 0x891c
#define SIOCGIFMETRIC 0x891d
#define SIOCSIFMETRIC 0x891e
#define SIOCGIFMEM 0x891f
#define SIOCSIFMEM 0x8920
#define SIOCGIFMTU 0x8921
#define SIOCSIFMTU 0x8922
#define SIOCSIFNAME 0x8923
#define SIOCSIFHWADDR 0x8924
#define SIOCGIFENCAP 0x8925
#define SIOCSIFENCAP 0x8926
#define SIOCGIFHWADDR 0x8927
#define SIOCGIFSLAVE 0x8929
#define SIOCSIFSLAVE 0x8930
#define SIOCADDMULTI 0x8931
#define SIOCDELMULTI 0x8932
#define SIOCGIFINDEX 0x8933
#define SIOGIFINDEX SIOCGIFINDEX
#define SIOCSIFPFLAGS 0x8934
#define SIOCGIFPFLAGS 0x8935
#define SIOCDIFADDR 0x8936
#define SIOCSIFHWBROADCAST 0x8937
#define SIOCGIFCOUNT 0x8938

void net_init(void);

SYSCALL_DECLARE(socket, int domain, int type, int protocol);
SYSCALL_DECLARE(connect, int sockfd, const struct sockaddr *name, socklen_t namelen);
SYSCALL_DECLARE(bind, int sockfd, const struct sockaddr *addr, socklen_t addrlen);
SYSCALL_DECLARE(listen, int sockfd, int backlog);
SYSCALL_DECLARE(accept, int sockfd, struct sockaddr *addr, socklen_t *addrlen);
SYSCALL_DECLARE(recv, int sockfd, void *mem, size_t len, int flags);
SYSCALL_DECLARE(recvfrom, int sockfd, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
SYSCALL_DECLARE(send, int sockfd, const void *dataptr, size_t size, int flags);
SYSCALL_DECLARE(sendto, int sockfd, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
SYSCALL_DECLARE(setsockopt, int sockfd, int level, int optname, const void *optval, socklen_t optlen);

#endif /* NET_H */
