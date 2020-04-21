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


static struct file_operations sockops = {
        .open = NULL,
        .close = close_sock,
        .read = read_sock,
        .write = write_sock,
        .mount = NULL,
        .readdir = NULL,
        .stat = NULL,
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

int do_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    int lwip_fd = get_lwip_fd(sockfd);

    return lwip_setsockopt(lwip_fd, level, optname, optval, optlen);
}




