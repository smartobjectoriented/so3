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

#include <network.h>
#include <schedule.h>
#include <vfs.h>
#include <process.h>
#include <heap.h>
#include <errno.h>
#include <process.h>
#include <mutex.h>
#include <string.h>
#include <dirent.h>

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netif.h>
#include <lwip/netifapi.h>
#include <device/network.h>

static void network_tcpip_done(void *args)
{
        network_devices_init();
}


void network_init(void)
{
        tcpip_init(network_tcpip_done, NULL);
}



// TODO: MOVE
/**************************** TEMP NET FS ***************************************/
int read_sock(int fd, void *buffer, int count)
{
        int lwip_fd = get_lwip_fd(fd);

        return lwip_read(lwip_fd, buffer, count);
}

int write_sock(int fd, const void *buffer, int count)
{
        int lwip_fd = get_lwip_fd(fd);

        return lwip_write(lwip_fd, buffer, count);
}

int close_sock(int fd)
{
        int lwip_fd = get_lwip_fd(fd);

        return lwip_close(lwip_fd);
}


// TODO move
#define SIOCADDRT       0x890B
#define SIOCDELRT       0x890C
#define SIOCRTMSG       0x890D

#define SIOCGIFNAME     0x8910
#define SIOCSIFLINK     0x8911
#define SIOCGIFCONF     0x8912
#define SIOCGIFFLAGS    0x8913
#define SIOCSIFFLAGS    0x8914
#define SIOCGIFADDR     0x8915
#define SIOCSIFADDR     0x8916
#define SIOCGIFDSTADDR  0x8917
#define SIOCSIFDSTADDR  0x8918
#define SIOCGIFBRDADDR  0x8919
#define SIOCSIFBRDADDR  0x891a
#define SIOCGIFNETMASK  0x891b
#define SIOCSIFNETMASK  0x891c
#define SIOCGIFMETRIC   0x891d
#define SIOCSIFMETRIC   0x891e
#define SIOCGIFMEM      0x891f
#define SIOCSIFMEM      0x8920
#define SIOCGIFMTU      0x8921
#define SIOCSIFMTU      0x8922
#define SIOCSIFNAME     0x8923
#define SIOCSIFHWADDR   0x8924
#define SIOCGIFENCAP    0x8925
#define SIOCSIFENCAP    0x8926
#define SIOCGIFHWADDR   0x8927
#define SIOCGIFSLAVE    0x8929
#define SIOCSIFSLAVE    0x8930
#define SIOCADDMULTI    0x8931
#define SIOCDELMULTI    0x8932
#define SIOCGIFINDEX    0x8933
#define SIOGIFINDEX     SIOCGIFINDEX
#define SIOCSIFPFLAGS   0x8934
#define SIOCGIFPFLAGS   0x8935
#define SIOCDIFADDR     0x8936
#define SIOCSIFHWBROADCAST 0x8937
#define SIOCGIFCOUNT    0x8938

// todo redefine as ifreq
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
                //struct ifmap ifru_map;
                char ifru_slave[IFNAMSIZ];
                char ifru_newname[IFNAMSIZ];
                char *ifru_data;
        } ifr_ifru;
};


// TODO

struct sockaddr_in_usr {
        u16 sin_family;
        in_port_t sin_port;
        struct in_addr sin_addr;
        uint8_t sin_zero[8];
};

/**
 * Adapt a userspace sockaddr to a lwip one.
 * Iwip sockaddr have a sa_len field as first byte
 * @param usr
 * @param lwip
 */
struct sockaddr *user_to_lwip_sockadd(struct sockaddr_in_usr *usr, struct sockaddr_in *lwip)
{
        if(usr == NULL){
                return NULL;
        }

        memset(lwip, 0, sizeof(struct sockaddr));

        lwip->sin_len = sizeof(struct sockaddr);
        lwip->sin_family = usr->sin_family;
        lwip->sin_port = usr->sin_port;
        lwip->sin_addr = usr->sin_addr;

        return (struct sockaddr *)lwip;
}

// TODO
int ioctl_sock(int fd, unsigned long cmd, unsigned long args)
{
        int lwip_fd = get_lwip_fd(fd);
        int id, index;
        char *hwaddr;
        struct ifreq2 *ifreq = NULL;
        struct netif *netif = NULL;
        struct sockaddr_in *addr = NULL;

        // LwIP handeled the ioctl cmd
        if (!lwip_ioctl(lwip_fd, cmd, (void *) args)) {
                return 0;
        }


        switch (cmd) {
        case SIOCGIFNAME:
                if (!args) {
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                id = ifreq->ifr_ifru.ifru_ivalue;
                netif = netif_get_by_index((u8_t) id + 1); // LWIP index start a 1
                if (netif == NULL) {
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }


                // Lwip netid name is always 2 chars followed by an id
                sprintf(ifreq->ifrn_name, "%c%c%d",
                        netif->name[0],
                        netif->name[1],
                        id);


                return 0;
        case SIOCGIFINDEX:
                if (!args) {
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                index = netif_name_to_index(ifreq->ifrn_name);

                if (index == NETIF_NO_INDEX) {
                        set_errno(EINVAL); //TODO change
                        return -1;
                }

                ifreq->ifr_ifru.ifru_ivalue = index - 1;
                return 0;


        case SIOCGIFHWADDR:
                if (!args) {
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                netif = netif_find(ifreq->ifrn_name);
                if (netif == NULL) {
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }

                ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
                ifreq->ifr_ifru.ifru_addr.sa_len = 4; //IPV4

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
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                netif = netif_find(ifreq->ifrn_name);
                if (netif == NULL) {
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }

                ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
                ifreq->ifr_ifru.ifru_addr.sa_len = 4; //IPV4

                addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;
                memcpy(&addr->sin_addr.s_addr, &netif->ip_addr.addr, 4);


                return 0;
        case SIOCSIFADDR:
                if (!args) {
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                netif = netif_find(ifreq->ifrn_name);
                if (netif == NULL) {
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }

                addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;

                netif_set_ipaddr(netif, &(ip4_addr_t){
                        .addr = addr->sin_addr.s_addr
                });

                return 0;

        case SIOCGIFBRDADDR:
                if (!args) {
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                netif = netif_find(ifreq->ifrn_name);
                if (netif == NULL) {
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }

                ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
                ifreq->ifr_ifru.ifru_addr.sa_len = 4; //IPV4

                addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;
                addr->sin_addr.s_addr = netif->ip_addr.addr | ~netif->netmask.addr;

                return 0;
        case SIOCSIFBRDADDR:
                // LwIP automaticaly compute broadcast address
                return 0;
        case SIOCGIFNETMASK:
                if (!args) {
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                netif = netif_find(ifreq->ifrn_name);
                if (netif == NULL) {
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }

                ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
                ifreq->ifr_ifru.ifru_addr.sa_len = 4; //IPV4

                addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;
                addr->sin_addr.s_addr = netif->netmask.addr;

                return 0;
        case SIOCSIFNETMASK:
                if (!args) {
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                netif = netif_find(ifreq->ifrn_name);
                if (netif == NULL) {
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }

                addr = (struct sockaddr_in *) &ifreq->ifr_ifru.ifru_addr;

                if(!ip4_addr_netmask_valid(addr->sin_addr.s_addr)){
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }

                netif_set_netmask(netif, &(ip4_addr_t){
                        .addr = addr->sin_addr.s_addr
                });

                return 0;
        case SIOCGIFMTU:
                if (!args) {
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                netif = netif_find(ifreq->ifrn_name);
                if (netif == NULL) {
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }

                ifreq->ifr_ifru.ifru_mtu = netif->mtu;

                return 0;
        case SIOCSIFMTU:
                if (!args) {
                        set_errno(EINVAL);
                        return -1;
                }
                ifreq = (struct ifreq2 *) args;

                netif = netif_find(ifreq->ifrn_name);
                if (netif == NULL) {
                        set_errno(EINVAL); // TODO edit
                        return -1;
                }

                netif->mtu = ifreq->ifr_ifru.ifru_mtu;

                return 0;

        default:
                return -1;
        }
}


static struct file_operations sockops = {
        .open = NULL,
        .close = close_sock,
        .read = read_sock,
        .write = write_sock,
        .mount = NULL,
        .readdir = NULL,
        .stat = NULL,
        .ioctl = ioctl_sock
};

struct file_operations *register_sock(void)
{
        return &sockops;
}




/**************************** Syscall implementation ****************************/

/*
 * Mapping between internal fd and vfs fd
 */
int lwip_fds[MAX_FDS];


int get_lwip_fd(int gfd)
{
        if (gfd < MAX_FDS) {
                return lwip_fds[gfd];
        } else {
                return -1;
        }
}

/*
 *
 */
int do_socket(int domain, int type, int protocol)
{
        int fd, gfd, lwip_fd;
        struct file_operations *fops;

        // TODO LOCK

        /* FIXME: Should find the mounted point regarding the path */
        /* At the moment... */
        fops = register_sock();

        /* vfs_open is already clean fops and open_fds */
        fd = vfs_open(NULL, fops, VFS_TYPE_DEV_SOCK);

        if (fd < 0) {
                /* fd already open */
                set_errno(EBADF);
                // TODO UNLOCK
                return -1;
        }

        /* Get index of open_fds*/
        gfd = current()->pcb->fd_array[fd];

        vfs_set_open_mode(gfd, 0);

        lwip_fd = lwip_socket(domain, type, protocol);

        if (lwip_fd < 0) {
                do_close(fd);
                return lwip_fd;
        }

        // TODO check fd ok
        lwip_fds[gfd] = lwip_fd;



        // TODO UNLOCK

        return fd;
}

int do_connect(int sockfd, const struct sockaddr *addr, socklen_t namelen)
{
        struct sockaddr_in addr_lwip;
        struct sockaddr *addr_ptr;

        int lwip_fd = get_lwip_fd(sockfd);

        addr_ptr = user_to_lwip_sockadd((struct sockaddr_in_usr *) addr, &addr_lwip);

        return lwip_connect(lwip_fd, addr_ptr, namelen);
}

int do_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
        struct sockaddr_in addr_lwip;
        struct sockaddr *addr_ptr;

        int lwip_fd = get_lwip_fd(sockfd);

        addr_ptr = user_to_lwip_sockadd((struct sockaddr_in_usr *) addr, &addr_lwip);

        return lwip_bind(lwip_fd, addr_ptr, addrlen);
}

int do_listen(int sockfd, int backlog)
{
        int lwip_fd = get_lwip_fd(sockfd);

        return lwip_listen(lwip_fd, backlog);
}

int do_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
        int fd, gfd, lwip_fd, lwip_bind_fd;
        struct file_operations *fops;
        struct sockaddr_in addr_lwip;
        struct sockaddr *addr_ptr;


        lwip_fd = get_lwip_fd(sockfd);

        addr_ptr = user_to_lwip_sockadd((struct sockaddr_in_usr *) addr, &addr_lwip);



        // TODO LOCK

        /* FIXME: Should find the mounted point regarding the path */
        /* At the moment... */
        fops = register_sock();

        /* vfs_open is already clean fops and open_fds */
        fd = vfs_open(NULL, fops, VFS_TYPE_DEV_SOCK);

        if (fd < 0) {
                /* fd already open */
                set_errno(EBADF);
                // TODO UNLOCK
                return -1;
        }

        /* Get index of open_fds*/
        gfd = current()->pcb->fd_array[fd];

        vfs_set_open_mode(gfd, 0);

        lwip_bind_fd = lwip_accept(lwip_fd, addr_ptr, addrlen);

        if (lwip_fd < 0) {
                do_close(fd);
                return lwip_fd;
        }

        // TODO check fd ok
        lwip_fds[gfd] = lwip_bind_fd;


        // TODO UNLOCK

        return fd;


}

int do_recv(int sockfd, void *mem, size_t len, int flags)
{
        int lwip_fd = get_lwip_fd(sockfd);

        return lwip_recv(lwip_fd, mem, len, flags);
}

int do_recvfrom(int sockfd, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{

        int lwip_fd = get_lwip_fd(sockfd);

        return lwip_recvfrom(lwip_fd, mem, len, flags, from, fromlen);
}


int do_send(int sockfd, const void *dataptr, size_t size, int flags)
{
        int lwip_fd = get_lwip_fd(sockfd);

        return lwip_send(lwip_fd, dataptr, size, flags);
}
/*
struct sockaddr_in {
    u8_t            sin_len; //8
    sa_family_t     sin_family; //8
    in_port_t       sin_port; //16
    struct in_addr  sin_addr; //32
#define SIN_ZERO_LEN 8
    char            sin_zero[SIN_ZERO_LEN];
};*/

/**
struct sockaddr_in {
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	uint8_t sin_zero[8];
};
 */



int do_sendto(int sockfd, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen)
{
        struct sockaddr_in to_lwip;
        int lwip_fd = get_lwip_fd(sockfd);

        user_to_lwip_sockadd((struct sockaddr_in_usr *) to, &to_lwip);


        return lwip_sendto(lwip_fd, dataptr, size, flags, (struct sockaddr *) &to_lwip, tolen);
}


int do_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
        int lwip_fd = get_lwip_fd(sockfd);

        return lwip_setsockopt(lwip_fd, level, optname, optval, optlen);
}


