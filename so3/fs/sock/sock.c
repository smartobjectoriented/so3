//
// Created by julien on 4/21/20.
//

#include <network.h>

int read_sock(int fd, void *buffer, int count)
{
    struct fat_entry *ptrent = (struct fat_entry *) vfs_get_privdata(fd);
    int rc = 0;
    int bread = 0;

    if (!ptrent) {
        DBG("Error while reading fd %d\n", fd);
        return -1;
    }

    if (ptrent->tentry != TYPE_FILE) {
        return -EINVAL;
    }

    if ((rc = f_read(&ptrent->entry.file, buffer, count, (unsigned *) &bread))) {
        return -rc;
    }

    return bread;
}

int write_sock(int fd, void *buffer, int count)
{
    struct fat_entry *ptrent = (struct fat_entry *) vfs_get_privdata(fd);
    int rc = 0;
    int bwritten = 0;

    if (!ptrent) {
        DBG("Error while writing fd %d\n", fd);
        return -1;
    }

    if (ptrent->tentry != TYPE_FILE) {
        return -EINVAL;
    }

    if ((rc = f_write(&ptrent->entry.file, buffer, count, (unsigned *) &bwritten))) {
        return -rc;
    }

    rc = f_sync(&ptrent->entry.file);

    return bwritten;
}

int close_sock(int fd)
{
    int rc;
    struct fat_entry *ptrent = (struct fat_entry *) vfs_get_privdata(fd);

    switch (ptrent->tentry) {
        case TYPE_FILE:
            if ((rc = f_close(&ptrent->entry.file))) {
                return -rc;
            }
            break;
        case TYPE_FOLDER:
            if ((rc = f_closedir(&ptrent->entry.dir.dir_ctx))) {
                return -rc;
            }
            break;
        default:
            return -EBADFD;
    }

    free(ptrent);
    return 0;
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

struct file_operations *register_sock(void)
{
    return &sockops;
}
