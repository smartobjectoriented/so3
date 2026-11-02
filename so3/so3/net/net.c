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

#include <net.h>
#include <schedule.h>
#include <vfs.h>
#include <process.h>
#include <heap.h>
#include <errno.h>
#include <process.h>
#include <mutex.h>
#include <string.h>
#include <dirent.h>
#include <initcall.h>

#include <net/lwip/tcpip.h>
#include <net/lwip/sockets.h>
#include <net/lwip/netif.h>
#include <net/lwip/netifapi.h>

#include <device/net.h>

/*
 * Mapping between internal fd and vfs fd
 */
int lwip_fds[MAX_FDS];
int lwip_errno = 0;

/**
 * Convert lwIP function calls return value into syscall return value.
 *
 * @param ret return value of a lwip function call
 * @return ret if function is success and -lwip_errno if not.
 */
static int lwip_return(int ret)
{
	if (ret < 0) {
		return -lwip_errno;
	}

	return ret;
}

/**
 * lwip_fds[] is indexed by GLOBAL file descriptor.
 *
 * @param Global file descriptor (gfd)
 * @return Associated socket ID from lwip
 */
static int get_lwip_fd_from_gfd(int gfd)
{
	if ((gfd < 0) || (gfd >= MAX_FDS))
		return -1;

	return lwip_fds[gfd];
}

/**
 *
 * @param Local file descriptor (fd)
 * @return Associated socket ID from lwip
 */
static int get_lwip_fd(int fd)
{
	if ((fd < 0) || (fd >= FD_MAX))
		return -1;

	/* Get the gfd from this fd */
	return get_lwip_fd_from_gfd(current()->pcb->fd_array[fd]);
}

/**************************** Network subsystem ***************************************/

/*
 * The file operations below are the ones the VFS calls, and it passes them the
 * GLOBAL descriptor (see the fops->read/write/close call sites in fs/vfs.c) —
 * unlike the socket syscalls, which get the process-local one. Translating a
 * gfd a second time used to land on an unrelated entry of the process fd table,
 * or on -1 once close() had already released it: lwip_close() then never
 * reached the socket, leaking its pcb and every packet queued on it.
 */

int read_sock(int gfd, void *buffer, int count)
{
	int ret;
	int lwip_fd = get_lwip_fd_from_gfd(gfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	ret = lwip_read(lwip_fd, buffer, count);
	return lwip_return(ret);
}

int write_sock(int gfd, const void *buffer, int count)
{
	int ret;
	int lwip_fd = get_lwip_fd_from_gfd(gfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	ret = lwip_write(lwip_fd, buffer, count);
	return lwip_return(ret);
}

int close_sock(int gfd)
{
	int ret;
	int lwip_fd = get_lwip_fd_from_gfd(gfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	/* Release the slot before the socket is gone, so a gfd handed out again
	 * later cannot be mistaken for this socket. */
	lwip_fds[gfd] = -1;

	ret = lwip_close(lwip_fd);
	return lwip_return(ret);
}

#warning redefine as ifreq
struct ifreq2 {
	char ifrn_name[16];
	union {
		struct sockaddr ifru_addr;
		struct sockaddr ifru_dstaddr;
		struct sockaddr ifru_broadaddr;
		struct sockaddr ifru_netmask;
		struct sockaddr ifru_hwaddr;
		short int ifru_flags;
		int ifru_ivalue;
		int ifru_mtu;
		/* struct ifmap ifru_map; */
		char ifru_slave[IFNAMSIZ];
		char ifru_newname[IFNAMSIZ];
		char *ifru_data;
	} ifr_ifru;
};

/**
 * Adapt a userspace sockaddr to a lwip one.
 * Iwip sockaddr have a sa_len field as first byte
 * @param usr
 * @param lwip
 */
struct sockaddr *user_to_lwip_sockadd(const struct usr_sockaddr_in *usr, struct sockaddr_in *lwip)
{
	if (usr == NULL) {
		return NULL;
	}

	memset(lwip, 0, sizeof(struct sockaddr_in));

	lwip->sin_len = sizeof(struct sockaddr_in);
	lwip->sin_family = usr->sin_family;
	lwip->sin_port = usr->sin_port;
	lwip->sin_addr = usr->sin_addr;

	return (struct sockaddr *) lwip;
}

/**
 * Adapt a lwip sockaddr back to a userspace one.
 * The two layouts differ by lwip's leading sa_len byte, so the peer address
 * an lwip call filled in cannot simply be memcpy'd out to user space.
 * @param lwip
 * @param usr
 */
void lwip_to_user_sockadd(const struct sockaddr_in *lwip, struct usr_sockaddr_in *usr)
{
	if (usr == NULL) {
		return;
	}

	memset(usr, 0, sizeof(struct usr_sockaddr_in));

	usr->sin_family = lwip->sin_family;
	usr->sin_port = lwip->sin_port;
	usr->sin_addr = lwip->sin_addr;
}

int ioctl_sock(int fd, unsigned long cmd, unsigned long args)
{
	int ret;
	int lwip_fd = get_lwip_fd(fd);
	int id, index;
	char *hwaddr;
	struct ifreq2 *ifreq = NULL;
	struct netif *netif = NULL;
	struct sockaddr_in *addr = NULL;

	if (lwip_fd < 0) {
		return -EBADF;
	}

	/* LwIP handeled the ioctl cmd */
	ret = lwip_ioctl(lwip_fd, cmd, (void *) args);
	if ((ret >= 0) || (lwip_errno != ENOSYS)) {
		return lwip_return(ret);
	}

	switch (cmd) {
	case SIOCGIFNAME:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		id = ifreq->ifr_ifru.ifru_ivalue;
		netif = netif_get_by_index((u8_t) id + 1); /* LWIP index start a 1 */
		if (netif == NULL) {
			return -EINVAL;
		}

		/* Lwip netid name is always 2 chars followed by an id */
		sprintf(ifreq->ifrn_name, "%c%c%d", netif->name[0], netif->name[1], id);

		return 0;

	case SIOCGIFINDEX:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		index = netif_name_to_index(ifreq->ifrn_name);

		if (index == NETIF_NO_INDEX) {
			return -EINVAL;
		}

		ifreq->ifr_ifru.ifru_ivalue = index - 1;
		return 0;

	case SIOCGIFHWADDR:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		netif = netif_find(ifreq->ifrn_name);
		if (netif == NULL) {
			return -EINVAL;
		}

		ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
		ifreq->ifr_ifru.ifru_addr.sa_len = 4; /* IPV4 */

		hwaddr = ifreq->ifr_ifru.ifru_hwaddr.sa_data;
		hwaddr[0] = netif->hwaddr[0];
		hwaddr[1] = netif->hwaddr[1];
		hwaddr[2] = netif->hwaddr[2];
		hwaddr[3] = netif->hwaddr[3];
		hwaddr[4] = netif->hwaddr[4];
		hwaddr[5] = netif->hwaddr[5];

		return 0;

	case SIOCGIFADDR:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		netif = netif_find(ifreq->ifrn_name);
		if (netif == NULL) {
			return -EINVAL;
		}

		ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
		ifreq->ifr_ifru.ifru_addr.sa_len = 4; /* IPV4 */

		addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;
		memcpy(&addr->sin_addr.s_addr, &netif->ip_addr.addr, 4);

		return 0;

	case SIOCSIFADDR:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		netif = netif_find(ifreq->ifrn_name);
		if (netif == NULL) {
			return -EINVAL;
		}

		addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;

		netif_set_ipaddr(netif, &(ip4_addr_t) { .addr = addr->sin_addr.s_addr });

		return 0;

	case SIOCGIFBRDADDR:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		netif = netif_find(ifreq->ifrn_name);
		if (netif == NULL) {
			return -EINVAL;
		}

		ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
		ifreq->ifr_ifru.ifru_addr.sa_len = 4; /* IPV4 */

		addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;
		addr->sin_addr.s_addr = netif->ip_addr.addr | ~netif->netmask.addr;

		return 0;
	case SIOCSIFBRDADDR:
		/* LwIP automaticaly compute broadcast address */
		return 0;

	case SIOCGIFNETMASK:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		netif = netif_find(ifreq->ifrn_name);
		if (netif == NULL) {
			return -EINVAL;
		}

		ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
		ifreq->ifr_ifru.ifru_addr.sa_len = 4; /* IPV4 */

		addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;
		addr->sin_addr.s_addr = netif->netmask.addr;

		return 0;

	case SIOCSIFNETMASK:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		netif = netif_find(ifreq->ifrn_name);
		if (netif == NULL) {
			return -EINVAL;
		}

		addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;

		if (!ip4_addr_netmask_valid(addr->sin_addr.s_addr)) {
			return -EINVAL;
		}

		netif_set_netmask(netif, &(ip4_addr_t) { .addr = addr->sin_addr.s_addr });

		return 0;

	case SIOCGIFMTU:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		netif = netif_find(ifreq->ifrn_name);
		if (netif == NULL) {
			return -EINVAL;
		}

		ifreq->ifr_ifru.ifru_mtu = netif->mtu;

		return 0;

	case SIOCSIFMTU:
		if (!args) {
			return -EINVAL;
		}
		ifreq = (struct ifreq2 *) args;

		netif = netif_find(ifreq->ifrn_name);
		if (netif == NULL) {
			return -EINVAL;
		}

		netif->mtu = ifreq->ifr_ifru.ifru_mtu;

		return 0;

	default:
		return -EINVAL;
	}
}

static struct file_operations sockops = { .open = NULL,
					  .close = close_sock,
					  .read = read_sock,
					  .write = write_sock,
					  .mount = NULL,
					  .readdir = NULL,
					  .stat = NULL,
					  .ioctl = ioctl_sock };

struct file_operations *register_sock(void)
{
	return &sockops;
}

/**************************** Syscall implementation ****************************/

/* Linux's flags of a socket's type, which lwIP does not know. */

#define USR_SOCK_NONBLOCK 04000
#define USR_SOCK_CLOEXEC 02000000

SYSCALL_DEFINE3(socket, int, domain, int, type, int, protocol)
{
	int fd, gfd, lwip_fd, nonblock = !!(type & USR_SOCK_NONBLOCK);
	struct file_operations *fops;

	type &= ~(USR_SOCK_NONBLOCK | USR_SOCK_CLOEXEC);

	fops = register_sock();

	/* vfs_open is already clean fops and open_fds */
	fd = vfs_open(NULL, fops, VFS_TYPE_DEV_SOCK);

	if (fd < 0) {
		/* fd already open */
		return -EBADF;
	}

	/* Get index of open_fds*/
	gfd = current()->pcb->fd_array[fd];

	vfs_set_open_mode(gfd, 0);

	lwip_fd = lwip_socket(domain, type, protocol);

	if (lwip_fd < 0) {
		sys_do_close(fd);
		return lwip_return(lwip_fd);
	}

	/* lwIP's O_NONBLOCK is not the VFS's: FIONBIO is the same on both sides. */

	if (nonblock)
		lwip_ioctl(lwip_fd, FIONBIO, &nonblock);

	lwip_fds[gfd] = lwip_fd;

	return fd;
}

SYSCALL_DEFINE3(connect, int, sockfd, const struct usr_sockaddr_in *, addr, socklen_t, namelen)
{
	int ret;
	struct sockaddr_in addr_lwip;
	struct sockaddr *addr_ptr;

	int lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	addr_ptr = user_to_lwip_sockadd(addr, &addr_lwip);

	ret = lwip_connect(lwip_fd, addr_ptr, namelen);
	return lwip_return(ret);
}

SYSCALL_DEFINE3(bind, int, sockfd, const struct usr_sockaddr_in *, addr, socklen_t, addrlen)
{
	int ret;
	struct sockaddr_in addr_lwip;
	struct sockaddr *addr_ptr;

	int lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	addr_ptr = user_to_lwip_sockadd(addr, &addr_lwip);

	ret = lwip_bind(lwip_fd, addr_ptr, addrlen);
	return lwip_return(ret);
}

SYSCALL_DEFINE2(listen, int, sockfd, int, backlog)
{
	int ret;
	int lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	ret = lwip_listen(lwip_fd, backlog);
	return lwip_return(ret);
}

SYSCALL_DEFINE3(accept, int, sockfd, struct usr_sockaddr_in *, addr, socklen_t *, addrlen)
{
	int fd, gfd, lwip_fd, lwip_bind_fd;
	struct file_operations *fops;
	struct sockaddr_in addr_lwip;
	struct sockaddr *addr_ptr;

	lwip_fd = get_lwip_fd(sockfd);
	if (lwip_fd < 0) {
		return -EBADF;
	}

	addr_ptr = user_to_lwip_sockadd(addr, &addr_lwip);

	fops = register_sock();

	/* vfs_open is already clean fops and open_fds */
	fd = vfs_open(NULL, fops, VFS_TYPE_DEV_SOCK);

	if (fd < 0) {
		/* fd already open */
		return -EBADF;
	}

	/* Get index of open_fds*/
	gfd = current()->pcb->fd_array[fd];

	vfs_set_open_mode(gfd, 0);

	lwip_bind_fd = lwip_accept(lwip_fd, addr_ptr, addrlen);

	if (lwip_bind_fd < 0) {
		sys_do_close(fd);
		return lwip_return(lwip_bind_fd);
	}

	/*  TODO check fd ok */
	lwip_fds[gfd] = lwip_bind_fd;

	/* Copy the peer address back out, converting the layout */
	lwip_to_user_sockadd(&addr_lwip, addr);

	return fd;
}

SYSCALL_DEFINE4(recv, int, sockfd, void *, mem, size_t, len, int, flags)
{
	int ret;
	int lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	ret = lwip_recv(lwip_fd, mem, len, flags);
	return lwip_return(ret);
}

SYSCALL_DEFINE6(recvfrom, int, sockfd, void *, mem, size_t, len, int, flags, struct usr_sockaddr_in *, from, socklen_t *,
		fromlen)
{
	int ret;
	struct sockaddr_in from_lwip;
	struct sockaddr *from_ptr;
	int lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	from_ptr = user_to_lwip_sockadd(from, &from_lwip);

	ret = lwip_recvfrom(lwip_fd, mem, len, flags, from_ptr, fromlen);

	/* lwip_recvfrom() filled the SOURCE address into our local sockaddr;
	 * without this it never reached the caller (ping printed 0.0.0.0). */
	if (ret >= 0)
		lwip_to_user_sockadd(&from_lwip, from);

	return lwip_return(ret);
}

SYSCALL_DEFINE4(send, int, sockfd, const void *, dataptr, size_t, size, int, flags)
{
	int ret;
	int lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	ret = lwip_send(lwip_fd, dataptr, size, flags);
	return lwip_return(ret);
}

SYSCALL_DEFINE6(sendto, int, sockfd, const void *, dataptr, size_t, size, int, flags, const struct usr_sockaddr_in *, to,
		socklen_t, tolen)
{
	int ret;
	struct sockaddr_in to_lwip;
	struct sockaddr *to_ptr;
	int lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	to_ptr = user_to_lwip_sockadd(to, &to_lwip);

	ret = lwip_sendto(lwip_fd, dataptr, size, flags, to_ptr, tolen);
	return lwip_return(ret);
}

SYSCALL_DEFINE5(setsockopt, int, sockfd, int, level, int, optname, const void *, optval, socklen_t, optlen)
{
	int ret;
	int lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0) {
		return -EBADF;
	}

	ret = lwip_setsockopt(lwip_fd, level, optname, optval, optlen);
	return lwip_return(ret);
}

/* Linux's MSG_ flags, which lwIP numbers otherwise. */

#define USR_MSG_PEEK 0x02
#define USR_MSG_TRUNC 0x20
#define USR_MSG_DONTWAIT 0x40

/**
 * Translate the flags of a call from Linux's values to lwIP's.
 *
 * @param flags	the flags user space gave
 * @return lwIP's
 */
static int lwip_msg_flags(int flags)
{
	return ((flags & USR_MSG_PEEK) ? MSG_PEEK : 0) | ((flags & USR_MSG_DONTWAIT) ? MSG_DONTWAIT : 0);
}

SYSCALL_DEFINE3(recvmsg, int, sockfd, struct usr_msghdr *, umsg, int, flags)
{
	struct sockaddr_in from_lwip;
	struct msghdr msg = { 0 };
	int ret, lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0)
		return -EBADF;

	if (umsg->msg_name) {
		msg.msg_name = &from_lwip;
		msg.msg_namelen = sizeof(from_lwip);
	}
	msg.msg_iov = umsg->msg_iov;
	msg.msg_iovlen = umsg->msg_iovlen;

	ret = lwip_recvmsg(lwip_fd, &msg, lwip_msg_flags(flags));

	if (ret >= 0) {
		if (umsg->msg_name) {
			lwip_to_user_sockadd(&from_lwip, umsg->msg_name);
			umsg->msg_namelen = sizeof(struct usr_sockaddr_in);
		}
		umsg->msg_controllen = 0;
		umsg->msg_flags = (msg.msg_flags & MSG_TRUNC) ? USR_MSG_TRUNC : 0;
	}

	return lwip_return(ret);
}

SYSCALL_DEFINE3(sendmsg, int, sockfd, const struct usr_msghdr *, umsg, int, flags)
{
	struct sockaddr_in to_lwip;
	struct msghdr msg = { 0 };
	int ret, lwip_fd = get_lwip_fd(sockfd);

	if (lwip_fd < 0)
		return -EBADF;

	msg.msg_name = user_to_lwip_sockadd(umsg->msg_name, &to_lwip);
	msg.msg_namelen = msg.msg_name ? sizeof(to_lwip) : 0;
	msg.msg_iov = umsg->msg_iov;
	msg.msg_iovlen = umsg->msg_iovlen;

	ret = lwip_sendmsg(lwip_fd, &msg, lwip_msg_flags(flags));
	return lwip_return(ret);
}

/**
 * Wait for sockets to be ready, as poll() does, the events translated
 * between Linux's values and lwIP's. Only sockets are watched: another
 * descriptor is never ready.
 *
 * @param ufds		the descriptors, as user space gave them
 * @param nfds		how many
 * @param timeout	in ms, negative for no limit
 * @return how many are ready, or a negative errno
 */
static int poll_sockets(struct usr_pollfd *ufds, unsigned int nfds, int timeout)
{
	struct pollfd *fds;
	unsigned int i;
	int ret;

	if (nfds > FD_MAX)
		return -EINVAL;

	fds = malloc((nfds ? nfds : 1) * sizeof(*fds));
	if (!fds)
		return -ENOMEM;

	for (i = 0; i < nfds; i++) {
		fds[i].fd = (ufds[i].fd < 0) ? -1 : get_lwip_fd(ufds[i].fd);
		fds[i].events = ((ufds[i].events & USR_POLLIN) ? POLLIN : 0) | ((ufds[i].events & USR_POLLOUT) ? POLLOUT : 0);
		fds[i].revents = 0;
	}

	ret = lwip_poll(fds, nfds, timeout);

	for (i = 0; i < nfds; i++)
		ufds[i].revents =
			((fds[i].revents & POLLIN) ? USR_POLLIN : 0) | ((fds[i].revents & POLLOUT) ? USR_POLLOUT : 0) |
			((fds[i].revents & POLLERR) ? USR_POLLERR : 0) | ((fds[i].revents & POLLHUP) ? USR_POLLHUP : 0) |
			((fds[i].revents & POLLNVAL) ? USR_POLLNVAL : 0);

	free(fds);

	return lwip_return(ret);
}

SYSCALL_DEFINE3(poll, struct usr_pollfd *, fds, unsigned int, nfds, int, timeout)
{
	return poll_sockets(fds, nfds, timeout);
}

/* ppoll(), which arm64's musl calls for poll(); the signal mask is not kept. */

SYSCALL_DEFINE5(ppoll, struct usr_pollfd *, fds, unsigned int, nfds, const struct usr_timespec *, tmo, const void *, sigmask,
		size_t, sigsetsize)
{
	return poll_sockets(fds, nfds, tmo ? (int) (tmo->tv_sec * 1000 + tmo->tv_nsec / 1000000) : -1);
}

static void network_tcpip_done(void *args)
{
	network_devices_init();
}

void net_init(void)
{
	int gfd;

	/* 0 is a perfectly valid lwip socket id, so the zero-initialised table
	 * would make every descriptor look like socket 0. */
	for (gfd = 0; gfd < MAX_FDS; gfd++)
		lwip_fds[gfd] = -1;

	tcpip_init(network_tcpip_done, NULL);
}

REGISTER_POSTINIT(net_init);
