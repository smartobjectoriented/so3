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
#include <roxml.h>
#include <heap.h>
#include <list.h>

#include <device/timer.h>

#include "common.h"

LIST_HEAD(visits);
LIST_HEAD(known_soo_list);

/* ME ID related information */
#define ME_NAME_SIZE		40
#define ME_SHORTDESC_SIZE	1024

#define SPID_SIZE		16

#define MAX_ME_DOMAINS		5

typedef enum {
	ME_state_booting,
	ME_state_preparing,
	ME_state_living,
	ME_state_suspended,
	ME_state_migrating,
	ME_state_dormant,
	ME_state_killed,
	ME_state_terminated,
	ME_state_dead
} ME_state_t;

/*
 * Definition of ME ID information used by functions which need
 * to get a list of running MEs with their information.
 */
typedef struct {
	ME_state_t	state;

	uint32_t slotID;

	uint64_t spid;
	char name[ME_NAME_SIZE];
	char shortdesc[ME_SHORTDESC_SIZE];
} ME_id_t;

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


char *tryxml(void) {

	char *buffer = NULL;
	node_t *root = roxml_add_node(NULL, 0, ROXML_ELM_NODE, "xml", NULL);
	node_t *tmp = roxml_add_node(root, 0, ROXML_CMT_NODE, NULL, "sample XML file");
	tmp = roxml_add_node(root, 0, ROXML_ELM_NODE, "item", NULL);
	roxml_add_node(tmp, 0, ROXML_ATTR_NODE, "id", "42");
	tmp = roxml_add_node(tmp, 0, ROXML_ELM_NODE, "price", "24");
	roxml_commit_changes(root, NULL, &buffer, 1);
	roxml_close(root);

	printk("result: %s\n", buffer);

	return buffer;
}

void parsexml(char *buffer) {

	node_t *root = roxml_load_buf(buffer);

	node_t *xml =  roxml_get_chld(root, NULL,  0);
	node_t *cmt1 = roxml_get_cmt(xml, 0);
	node_t *item = roxml_get_chld(xml, NULL, 0);
	node_t *prop = roxml_get_attr(item, "id", 0);

	node_t *val = roxml_get_chld(item, NULL, 0);
	node_t *txt = roxml_get_txt(val, 0);

	printk("### cmt %s\n", roxml_get_content(cmt1, NULL, 0, NULL));
	printk("### prop %s\n", roxml_get_content(prop, NULL, 0, NULL));
	printk("### val %s\n", roxml_get_content(txt, NULL, 0, NULL));

	roxml_close(root);

}


/**
 * Prepare a XML message for sending to the UI app
 *
 * @param buffer	buffer allocated by the caller
 * @param id		unique ID which identifies the message
 * @param value		Message content
 */
void xml_prepare_message(char *buffer, char *id, char *value) {
	char *__buffer;
	node_t *root = roxml_add_node(NULL, 0, ROXML_ELM_NODE, "xml", NULL);
	node_t *messages, *msg;

	/* Adding attributes to xml node */
	roxml_add_node(root, 0, ROXML_ATTR_NODE, "version", "1.0");

	/* Adding the messages node */
	messages = roxml_add_node(root, 0, ROXML_ELM_NODE, "messages", NULL);

	/* Adding the message itself */
	msg = roxml_add_node(messages, 0, ROXML_ELM_NODE, "message", NULL);

	roxml_add_node(msg, 0, ROXML_ATTR_NODE, "to", id);

	roxml_add_node(msg, 0, ROXML_TXT_NODE, NULL, value);

	roxml_commit_changes(root, NULL, &__buffer, 1);

	strcpy(buffer, __buffer);

	roxml_release(RELEASE_LAST);
	roxml_close(root);

}

/**
 * Retrieve the content of an event message
 *
 * @param buffer	The source event message
 * @param id		The ID of this event
 * @param action	The action of this event message
 */
void xml_parse_event(char *buffer, char *id, char *action) {

	node_t *root, *xml;
	node_t *events, *event, *__from, *__action;

	root = roxml_load_buf(buffer);
	xml =  roxml_get_chld(root, NULL,  0);

	events = roxml_get_chld(xml, NULL, 0);
	event = roxml_get_chld(events, NULL, 0);
	__from = roxml_get_attr(event, "from", 0);
	__action = roxml_get_attr(event, "action", 0);


	strcpy(id, roxml_get_content(__from, NULL, 0, NULL));
	strcpy(action, roxml_get_content(__action, NULL, 0, NULL));

	roxml_release(RELEASE_LAST);
	roxml_close(root);

}

/**
 * Prepare a well-formated XML string which is compliant with the
 * table UI application.
 *
 * @param buffer	allocated by the caller, will contain the resulting string
 * @param ME_id_array	ME IDs array to process
 *
 */
void *xml_prepare_id_array(char *buffer, ME_id_t *ME_id_array) {
	uint32_t pos;
	char *__buffer;
	node_t *root, *messages, *msg, *me, *name, *shortdesc;
	char spid[SPID_SIZE];

	/* Adding attributes to xml node */
	root = roxml_add_node(NULL, 0, ROXML_ELM_NODE, "xml", NULL);
	roxml_add_node(root, 0, ROXML_ATTR_NODE, "version", "1.0");
	roxml_add_node(root, 0, ROXML_ATTR_NODE, "encoding", "UTF-8");

	/* Adding the messages node */
	messages = roxml_add_node(root, 0, ROXML_ELM_NODE, "mobile-entities", NULL);

	for (pos = 0; pos < MAX_ME_DOMAINS; pos++) {

		if (ME_id_array[pos].state != ME_state_dead) {

			/* Adding the message itself */
			me = roxml_add_node(messages, 0, ROXML_ELM_NODE, "mobile-entity", NULL);

			/* Add SPID */
			sprintf(spid, "%llx", ME_id_array[pos].spid);
			roxml_add_node(me, 0, ROXML_ATTR_NODE, "spid", spid);

			/* Add short name */
			name = roxml_add_node(me, 0, ROXML_ELM_NODE, "name", NULL);
			roxml_add_node(name, 0, ROXML_TXT_NODE, NULL, ME_id_array[pos].name);

			/* And the short description */
			shortdesc = roxml_add_node(me, 0, ROXML_ELM_NODE, "description", NULL);
			roxml_add_node(shortdesc, 0, ROXML_TXT_NODE, NULL, ME_id_array[pos].shortdesc);
		}

	}

	roxml_commit_changes(root, NULL, &__buffer, 1);

	strcpy(buffer, __buffer);

	roxml_release(RELEASE_LAST);
	roxml_close(root);

}

typedef struct {

	me_common_t me_common;
	char hello[20];

} ctrl_t;

/*
 * Main entry point of so3 app in kernel standalone configuration.
 * Mainly for debugging purposes.
 */
int app_thread_main(void *args)
{
	char *buffer;
	int i;
	char src[800];
	char id[100];
	char action[100];
	ME_id_t ME_id[MAX_ME_DOMAINS];

	agencyUID_t a1 = { .id = { 0x00, 0x00, 0x00, 0x00, 0x01 } };
	agencyUID_t a2 = { .id = { 0x00, 0x00, 0x00, 0x00, 0x05 } };
	agencyUID_t a3 = { .id = { 0x00, 0x00, 0x00, 0x00, 0x08 } };
	agencyUID_t a4 = { .id = { 0x00, 0x00, 0x00, 0x00, 0x10 } };
	agencyUID_t a5 = { .id = { 0x00, 0x00, 0x00, 0xbb, 0xbb } };
	agencyUID_t a6 = { .id = { 0x00, 0x00, 0x00, 0xbb, 0xbc } };
	agencyUID_t a7 = { .id = { 0x00, 0x00, 0x00, 0xbb, 0xbd } };


	agencyUID_t b1 = { .id = { 0x00, 0x00, 0x00, 0x01, 0x01 } };
	agencyUID_t b2 = { .id = { 0x00, 0x00, 0x00, 0x01, 0x05 } };
	agencyUID_t b3 = { .id = { 0x00, 0x00, 0x00, 0x01, 0x08 } };

	host_entry_t *host_entry;

	ctrl_t *ctrl = (ctrl_t *) get_contig_free_vpages(1);
	int nr;

	strcpy(ctrl->hello, "Hello the world\n");

dump_heap("A");

	new_host(&known_soo_list, &a5, NULL, 0);
	new_host(&known_soo_list, &a3, NULL, 0);
	new_host(&known_soo_list, &a4, ctrl->hello, strlen(ctrl->hello)+1);
	new_host(&known_soo_list, &a1, NULL, 0);
	new_host(&known_soo_list, &a2, NULL, 0);

	dump_hosts(&known_soo_list);

	del_host(&known_soo_list, &a1);

	sort_hosts(&known_soo_list);

	dump_hosts(&known_soo_list);

	lprintk("## concatenating...\n");
	nr = concat_hosts(&known_soo_list, (uint8_t *) &ctrl->me_common.soohosts);
	lprintk("### nr = %d\n", nr);

	expand_hosts(&visits,  (uint8_t *) &ctrl->me_common.soohosts, nr);
	dump_hosts(&visits);

	host_entry = find_host(&known_soo_list, &a4);
	ASSERT(host_entry);

	lprintk("--> %s\n", (char *) host_entry->priv);
	lprintk("--> %d\n", host_entry->priv_len);

	new_host(&known_soo_list, &a6, NULL, 0);
	new_host(&known_soo_list, &a7, NULL, 0);

	merge_hosts(&visits, &known_soo_list);
	lprintk("## after merge\n");

	dump_hosts(&visits);

	lprintk("## sorting...\n");

	sort_hosts(&known_soo_list);

	dump_hosts(&known_soo_list);

	//lprintk("## duplicating ..\n");

	//duplicate_hosts(&known_soo_list, &visits);

	clear_hosts(&visits);

	new_host(&visits, &a3, NULL, 0);
	new_host(&visits, &a2, NULL, 0);
	//new_host(&visits, &a1, NULL, 0);
	new_host(&visits, &a5, NULL, 0);
	new_host(&visits, &a4, NULL, 0);

	//new_host(&visits, &b1, NULL, 0);

	dump_hosts(&visits);

	lprintk("## comparing...\n");

	if (hosts_equals(&known_soo_list, &visits))
		lprintk("## EQUAL\n");
	else
		lprintk("!!! NOT EQUAL !!!\n");

	host_entry = find_host(&visits, &a5);

	if (host_entry) {
		lprintk("### found: "); lprintk_printlnUID(&host_entry->uid);
	} else
		lprintk("## NOT FOUND\n");

	clear_hosts(&visits);

	dump_hosts(&visits);

	clear_hosts(&known_soo_list);

dump_heap("C");
	while(1);

#if 0
	void *ptr;
#endif
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
	ptr = malloc(40);
	ptr = malloc(268);
	ptr = malloc(32);
	ptr = malloc(40);
	ptr = malloc(8);
	ptr = malloc(40);

	while (1);
#endif

#if 0

	for (i = 0; i < MAX_ME_DOMAINS; i++)
		ME_id[i].state = ME_state_dead;

	ME_id[0].state = ME_state_living;
	ME_id[0].spid = 0x1122334455667788ull;

	strcpy(ME_id[0].name, "SOO.heat");
	strcpy(ME_id[0].shortdesc, "A simple heater where you can only increase or decrease the temperature.");

	ME_id[1].state = ME_state_living;
	ME_id[1].spid = 0x8877665544332211ull;

	strcpy(ME_id[1].name, "SOO.blind");
	strcpy(ME_id[1].shortdesc, "A simple blind motor control.");

	xml_prepare_id_array(src, ME_id);

	printk("result: %s\n", src);

	while (1);

#endif
#if 0
	/* Example of a message */
	xml_prepare_message(src, "8b4-4941", "16.5 C");

	/* Exampe of an event */


	printk("result: %s\n", src);

	strcpy(src, "<xml version=\"1.0\" encoding=\"UTF-8\"><events><event from=\"69b5d97e-78c3-4179-80e0-dea6eaa564c2\" action=\"clickDown\"/></events></xml>");
	xml_parse_event(src, id, action);


	printk("## id: %s\n", id);
	printk("## action: %s\n", action);

	while(1);

#endif

#if 0
	buffer = tryxml();
	strcpy(src, buffer);

	parsexml(src);

	while(1);
#endif

#if 0
	while (1) {
		printk("## Waiting 1 sec...\n");
		msleep(1000);
	}
#endif

#if 0
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

