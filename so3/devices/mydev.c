#include <vfs.h>

#include <device/driver.h>

char internal_buffer[20];

static int mydev_write(int fd, const void *buffer, int count) {

	strcpy(internal_buffer, buffer);

	return count;
}

static int mydev_read(int fd, void *buffer, int count) {

	strcpy(buffer, internal_buffer);

	return strlen(internal_buffer)+1;
}

struct file_operations mydev_fops = {
	.write = mydev_write,
	.read = mydev_read
};

struct classdev mydev_cdev = {
	.class = "mydev",
	.type = VFS_TYPE_DEV_CHAR,
	.fops = &mydev_fops,
};


int mydev_init(dev_t *dev) {

	/* Register the mydev driver so it can be accessed from user space. */
	devclass_register(dev, &mydev_cdev);

	return 0;
}


REGISTER_DRIVER_POSTCORE("arm,mydev", mydev_init);
