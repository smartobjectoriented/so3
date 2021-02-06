/*
 * Copyright (C) 2014-2019 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <thread.h>
#include <mutex.h>
#include <vfs.h>
#include <process.h>
#include <delay.h>
#include <semaphore.h>

#include <device/timer.h>

struct mutex lock;

static volatile int count = 0;
int counter;

char serial_getc(void);
int serial_gets(char *buf, int len);
int serial_puts(char *buf, int len);

sem_t sem;

int sem_test_fn(void *arg) {
	char name[20];
	int id = *((int *) arg);

	sprintf(name, "thread_%d", id);

	while (true) {

		printk("## %s: entering...\n", name);
		sem_down(&sem);

		printk("## %s: doing some work during a certain time...\n", name);
		msleep(50);
		printk("## %s: finished.\n", name);
		sem_up(&sem);
		printk("## %s: left critical section.\n", name);

		/* Consistency check */
		printk("## %s: sem val: %d\n", name, sem.val);
	}

}

int thread_fn1(void *arg)
{
	int i;

	printk("%s: hello\n", __func__);

#if 0
	printk("acquiring lock for job %d\n", counter);
	mutex_lock(&lock);
#endif

	counter += 1;
	printk("\n Job %d started\n", counter);


	for (i = 0; i < 100000; i++)
		count++;

	printk("\n Job %d finished\n", counter);

#if 0
	mutex_unlock(&lock);
#endif


	return 0;
}

int thread_example(void *arg)
{
	long long ii = 0;

	printk("### entering thread_example.\n");

	for (ii = 0; ii < 100000; ii++)
		count++;

	return 0;
}

int fn(void *args) {
	int i = 0, ret;

	printk("Thread #1\n");
	while (1) {
		ret = sem_timeddown(&sem, MILLISECS(100));
		if (ret != 0)
			lprintk("!");
		else
			lprintk("*");
		//sem_down(&sem);

		msleep((i*10) % 1000);

		if (ret == 0)
			sem_up(&sem);
		//sem_up(&sem);

		printk("--> th 1: %d\n", i++);
	}

	return 0;
}

int fn1(void *args) {
	//int i = 0;

	printk("Thread #1\n");
	//sem_down(&sem);
	while (1) {
		sem_down(&sem);
		msleep(499);
		sem_up(&sem);

//		printk("--> th 1: %d\n", i++);
	}
 
	return 0;
}

int fn2(void *args) {
	//int i = 0
	int ret;

	printk("Thread #2\n");
	while (1) {
		ret = sem_timeddown(&sem, MILLISECS(500));

		printk("## ret = %d\n", ret);
		if (ret == 0)
			sem_up(&sem);
//		printk("--> th 2: %d\n", i++);
	}

	return 0;
}

extern int schedcount;

/*
 * Main entry point of so3 app in kernel standalone configuration.
 * Mainly for debugging purposes.
 */
int app_thread_main(void *args)
{
#if 0
	int ret;
	char buf[128];
	int fd;
	tcb_t *t1, *t2;
#endif

#if 0
	tcb_t *thread[10];
	int tid;
	int i;
	unsigned long flags;
#endif

#if 0
	do_ls("mmc", "0:1", FS_TYPE_FAT, "/");
#endif

#if 0
	fd = fd_open("hello.txt", O_RDONLY);
	printk("%s: fd = %d\n", __func__, fd);
	ret = fd_read(fd, buf, 128);
	printk("%s: read %d characters: %s\n", __func__, ret, buf);
#endif

#if 0
	mutex_init(&lock);
#endif

	/* Kernel never returns ! */
	printk("***********************************************\n");
	printk("Going to infinite loop...\n");
	printk("Kill Qemu with CTRL-a + x or reset the board\n");
	printk("***********************************************\n");
#if 0
	while (1) {
		printk("## Waiting 1 sec...\n");
		msleep(1000);
	}
#endif

#if 1
	{
		int i;

		sem_init(&sem);
		for (i = 0; i < 20; i++)
			kernel_thread(fn, "fn1", NULL, 0);
	}

#endif

#if 0 /* Another test code */
	{
		int id[50], i;

		sem_init(&sem);

		for (i = 0; i < 20; i++) {
			id[i] = i;
			kernel_thread(sem_test_fn, "sem", &id[i], 0);
		}

		while (true);
	}
#endif

	return 0;
}

