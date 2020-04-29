//
// Created by julien on 3/25/20.
//


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
#include <device/network.h>

static void network_tcpip_done(void *args) {


    network_devices_init();
    printk("done");

    // TODO remove test code
    /**struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = 90;
    addr.sin_addr.s_addr = inet_addr("192.168.1.1");

    int ret = 0;

    int fd = do_socket(AF_INET, SOCK_STREAM, 0);
    printk("%i", fd);

    ret = do_connect(fd, (struct sockaddr *) &addr, sizeof(addr));
    printk("%i", ret);

    ret = do_write(fd, "test", 4);
    printk("%i", ret);

    do_close(fd);*/


}


void network_init(void) {
    tcpip_init(network_tcpip_done, NULL);
}



// TODO: MOVE
/**************************** TEMP NET FS ***************************************/
int read_sock(int fd, void *buffer, int count) {
    int lwip_fd = get_lwip_fd(fd);

    return lwip_read(lwip_fd, buffer, count);

}

int write_sock(int fd, void *buffer, int count) {
    int lwip_fd = get_lwip_fd(fd);

    return lwip_write(lwip_fd, buffer, count);
}

int close_sock(int fd) {
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
int ioctl_sock(int fd, unsigned long cmd, unsigned long args) {
    int lwip_fd = get_lwip_fd(fd);
    int id;
    struct ifreq2* ifreq = NULL;
    struct netif* netif = NULL;
    struct sockaddr_in *addr = NULL;

    // LwIP handeled the ioctl cmd
    if(!lwip_ioctl(lwip_fd, cmd, (void*)args)){
        return 0;
    }


    switch (cmd){
        case SIOCGIFNAME:
            if (!args) {
                set_errno(EINVAL);
                return -1;
            }
            ifreq = (struct ifreq2*)args;

            id = ifreq->ifr_ifru.ifru_ivalue;
            netif = netif_get_by_index((u8_t)id + 1); // LWIP index start a 1
            if(netif == NULL){
                set_errno(EINVAL); // TODO edit
                return -1;
            }


            // Lwip netid name is always 2 charsma followed by an id
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
            ifreq = (struct ifreq2*)args;

            int index = netif_name_to_index(ifreq->ifrn_name);

            if(index == NETIF_NO_INDEX){
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
            ifreq = (struct ifreq2*)args;

            netif = netif_find(ifreq->ifrn_name);
            if(netif == NULL){
                set_errno(EINVAL); // TODO edit
                return -1;
            }

            ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
            ifreq->ifr_ifru.ifru_addr.sa_len = 4; //IPV4

            char* hwaddr = ifreq->ifr_ifru.ifru_hwaddr.sa_data;
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
            ifreq = (struct ifreq2*)args;

            netif = netif_find(ifreq->ifrn_name);
            if(netif == NULL){
                set_errno(EINVAL); // TODO edit
                return -1;
            }

            ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
            ifreq->ifr_ifru.ifru_addr.sa_len = 4; //IPV4

            addr = (struct sockaddr_in *)&ifreq->ifr_ifru.ifru_addr;
            addr->sin_addr.s_addr = netif->ip_addr.addr;

            return 0;

        case SIOCGIFBRDADDR:
            if (!args) {
                set_errno(EINVAL);
                return -1;
            }
            ifreq = (struct ifreq2*)args;

            netif = netif_find(ifreq->ifrn_name);
            if(netif == NULL){
                set_errno(EINVAL); // TODO edit
                return -1;
            }

            ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
            ifreq->ifr_ifru.ifru_addr.sa_len = 4; //IPV4

            addr = (struct sockaddr_in *)&ifreq->ifr_ifru.ifru_addr;
            addr->sin_addr.s_addr = netif->ip_addr.addr | ~netif->netmask.addr;

            return 0;

        case SIOCGIFNETMASK:
            if (!args) {
                set_errno(EINVAL);
                return -1;
            }
            ifreq = (struct ifreq2*)args;

            netif = netif_find(ifreq->ifrn_name);
            if(netif == NULL){
                set_errno(EINVAL); // TODO edit
                return -1;
            }

            ifreq->ifr_ifru.ifru_addr.sa_family = AF_INET;
            ifreq->ifr_ifru.ifru_addr.sa_len = 4; //IPV4

            addr = (struct sockaddr_in *)&ifreq->ifr_ifru.ifru_addr;
            addr->sin_addr.s_addr = netif->netmask.addr;

            return 0;

        case SIOCGIFMTU:
            if (!args) {
                set_errno(EINVAL);
                return -1;
            }
            ifreq = (struct ifreq2*)args;

            netif = netif_find(ifreq->ifrn_name);
            if(netif == NULL){
                set_errno(EINVAL); // TODO edit
                return -1;
            }

            ifreq->ifr_ifru.ifru_mtu = netif->mtu;

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

struct file_operations *register_sock(void) {
    return &sockops;
}




/**************************** Syscall implementation ****************************/

/**
 * Mapping between internal fd and vfs fd
 */
int lwip_fds[MAX_FDS];


int get_lwip_fd(int gfd) {
    if (gfd < MAX_FDS)
        return lwip_fds[gfd];
    else
        return -1;
}

/**
 *
 * @param domain
 * @param type SOCK_STREAM for TCP or SOCK_DGRAM for UDP
 * @param protocol Protocol ID
 * @return Scoket FD
 */
int do_socket(int domain, int type, int protocol) {
    int fd, gfd, lwip_fd;
    struct file_operations *fops;

    // TODO LOCK

    /* FIXME: Should find the mounted point regarding the path */
    /* At the moment... */
    fops = register_sock();

    /* vfs_open is already clean fops and open_fds */
    fd = vfs_open(fops, VFS_TYPE_SOCK);

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


    // TODO check fd ok
    lwip_fds[gfd] = lwip_fd;



    // TODO UNLOCK

    return fd;


}

int do_connect(int sockfd, const struct sockaddr *name, socklen_t namelen) {
    int lwip_fd = get_lwip_fd(sockfd);

    return lwip_connect(lwip_fd, name, namelen);
}

int do_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int lwip_fd = get_lwip_fd(sockfd);

    return lwip_bind(lwip_fd, addr, addrlen);
}

int do_listen(int sockfd, int backlog) {
    int lwip_fd = get_lwip_fd(sockfd);

    return lwip_listen(lwip_fd, backlog);
}

int do_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int lwip_fd = get_lwip_fd(sockfd);

    return lwip_accept(lwip_fd, addr, addrlen);
}

int do_recv(int sockfd, void *mem, size_t len, int flags) {
    int lwip_fd = get_lwip_fd(sockfd);

    return lwip_recv(lwip_fd, mem, len, flags);
}

int do_send(int sockfd, const void *dataptr, size_t size, int flags) {
    int lwip_fd = get_lwip_fd(sockfd);

    return lwip_send(lwip_fd, dataptr, size, flags);
}

int do_sendto(int sockfd, const void *dataptr, size_t size, int flags,
              const struct sockaddr *to, socklen_t tolen) {

    int lwip_fd = get_lwip_fd(sockfd);

    return lwip_sendto(lwip_fd, dataptr, size, flags, to, tolen);
}



/*ssize_t lwip_recv(int s, void *mem, size_t len, int flags);
ssize_t lwip_read(int s, void *mem, size_t len);
ssize_t lwip_readv(int s, const struct iovec *iov, int iovcnt);
ssize_t lwip_recvfrom(int s, void *mem, size_t len, int flags,
                      struct sockaddr *from, socklen_t *fromlen);
ssize_t lwip_recvmsg(int s, struct msghdr *message, int flags);
ssize_t lwip_send(int s, const void *dataptr, size_t size, int flags);
ssize_t lwip_sendmsg(int s, const struct msghdr *message, int flags);
ssize_t lwip_sendto(int s, const void *dataptr, size_t size, int flags,
                    const struct sockaddr *to, socklen_t tolen);*/

int do_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    int lwip_fd = get_lwip_fd(sockfd);

    return lwip_setsockopt(lwip_fd, level, optname, optval, optlen);
}


