/*
 * Copyright (C) 2016-2018 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2016-2018 Baptiste Delporte <bonel@bonel.net>
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

#if 0
#define DEBUG
#endif

#include <types.h>
#include <mutex.h>
#include <heap.h>
#include <thread.h>

#include <asm/cacheflush.h>

#include <device/irq.h>

#include <soo/err.h>
#include <soo/vbstore.h>
#include <soo/hypervisor.h>
#include <soo/console.h>
#include <soo/debug.h>
#include <soo/hypervisor.h>
#include <soo/soo.h>
#include <soo/evtchn.h>
#include <soo/avz.h>
#include <soo/debug/logbool.h>

#define PRINTF_BUFFER_SIZE 4096

struct vbs_handle {
	/* A list of replies. Currently only one will ever be outstanding. */
	struct list_head reply_list;

	/*
	 * Mutex ordering: transaction_mutex -> watch_mutex -> request_mutex.
	 *
	 * transaction_mutex must be held before incrementing
	 * transaction_count. The mutex is held when a suspend is in
	 * progress to prevent new transactions starting.
	 *
	 * When decrementing transaction_count to zero the wait queue
	 * should be woken up, the suspend code waits for count to
	 * reach zero.
	 */

	/* Protect transactions against save/restore. */
	struct mutex transaction_mutex;
	volatile uint32_t transaction_count;

	struct mutex request_mutex;

	struct completion watch_wait;

	/* To manage ongoing transactions when the ME must be suspended */
	struct mutex transaction_group_mutex;

	/* Protect watch (de)register against save/restore. */
	struct mutex watch_mutex;

	spinlock_t msg_list_lock;
};
static struct vbs_handle vbs_state;

/* Used to keep the levtchn during migration */
static unsigned int __vbstore_levtchn;

/* List of registered watches, and a lock to protect it. */
static LIST_HEAD(watches);
static LIST_HEAD(vbus_msg_standby_list);

static uint32_t vbus_msg_ID = 0;
/*
 * transactionID = 0 means that there is no attached transaction. Hence, when transaction_start() is called for the first time,
 * it will be directly incremented to 1.
 */
unsigned int transactionID = 0;


void vbs_dump_watches(void) {

	struct vbus_watch *w;

	printk("----------- VBstore watches dump --------------\n");

	list_for_each_entry(w, &watches, list)
		printk("  Node name: %s  pending: %d\n", w->node, w->pending);

	printk("--------------- end --------------------\n");

}

/* Local reference to shared vbstore page between us and vbstore */
volatile struct vbstore_domain_interface *__intf;

/**
 * vbs_write - low level write
 * @data: buffer to send
 * @len: length of buffer
 *
 * Returns 0 on success, error otherwise.
 */
static void vbs_write(const void *data, unsigned len)
{
	VBSTORE_RING_IDX prod;
	volatile char *dst;

	DBG("__intf->req_prod: %d __intf->req_pvt: %d __intf->req_cons: %d\n", __intf->req_prod, __intf->req_pvt, __intf->req_cons);

	/* Read indexes, then verify. */
	prod = __intf->req_pvt;

	/* Check if we are at the end of the ring, i.e. there is no place for len bytes */
	if (prod + len >= VBSTORE_RING_SIZE) {
		prod = __intf->req_pvt = 0;
		if (prod + len > __intf->req_cons)
			BUG();
	}

#warning assert that the ring is not full

	dst = &__intf->req[prod];

	/* Must write data /after/ reading the producer index. */
	smp_mb();

	memcpy((void *) dst, data, len);

	__intf->req_pvt += len;

	/* Other side must not see new producer until data is there. */
	smp_mb();
}

static void vbs_read(void *data, unsigned len)
{
	VBSTORE_RING_IDX cons;
	volatile const char *src;

	DBG("__intf->rsp_prod: %d __intf->rsp_pvt: %d __intf->rsp_cons: %d\n", __intf->rsp_prod, __intf->rsp_pvt, __intf->rsp_cons);

	/* Read indexes, then verify. */
	cons = __intf->rsp_cons;

	if (cons + len >= VBSTORE_RING_SIZE)
		cons = __intf->rsp_cons = 0;

	src = &__intf->rsp[cons];

	/* Must read data /after/ reading the producer index. */
	smp_mb();

	memcpy(data, (void *) src, len);

	__intf->rsp_cons += len;

	/* Other side must not see free space until we've copied out */
	smp_mb();

}

/*
 * Main function to send a message to vbstore. It is the only way to send a
 * message to vbstore. It follows a synchronous send/reply scheme.
 * A message may have several strings within the payload. These (sub-)strings are known as vectors (msgvec_t).
 */
static void *vbs_talkv(struct vbus_transaction t, vbus_msg_type_t type, const msgvec_t *vec, unsigned int num_vecs, unsigned int *len)
{
	vbus_msg_t msg;
	unsigned int i;
	char *payload	= NULL;
	char *__payload_pos = NULL;
	uint32_t flags;

	/* Interrupts must be enabled because we expect an (asynchronous) reply from the peer.*/
	BUG_ON(local_irq_is_disabled());

	mutex_lock(&vbs_state.request_mutex);

	/* Message unique ID (on 32 bits) - 0 is a valid ID */
	msg.id = vbus_msg_ID++;

	msg.transactionID = t.id;

	/* Allocating a completion structure for this message - need to do that dynamically since the msg is part of the shared vbstore page
	 * and the size may vary depending on SMP is enabled or not.
	 */
	msg.u.reply_wait = (struct completion *) malloc(sizeof(struct completion));

	init_completion(msg.u.reply_wait);

	msg.type = type;
	msg.len = 0;
	for (i = 0; i < num_vecs; i++)
		msg.len += vec[i].len;

	vbs_write(&msg, sizeof(msg));

	payload = malloc(msg.len);
	if (payload == NULL) {
		printk("%s:%d ERROR cannot kmalloc the msg payload.\n", __func__, __LINE__);
		BUG();
	}
	memset(payload, 0, msg.len);

	__payload_pos = payload;
	for (i = 0; i < num_vecs; i++) {
		memcpy(__payload_pos, vec[i].base, vec[i].len);
		__payload_pos += vec[i].len;
	}

	DBG("Msg type: %d msg len: %d   content: %s\n", msg.type, msg.len, payload);

	vbs_write(payload, msg.len);
	free(payload);

	/* Store the current vbus_msg into the standby list for waiting the reply from vbstore. */
	flags = spin_lock_irqsave(&vbs_state.msg_list_lock);

	list_add_tail(&msg.list, &vbus_msg_standby_list);

	spin_unlock_irqrestore(&vbs_state.msg_list_lock, flags);

	__intf->req_prod = __intf->req_pvt;

	smp_mb();

	notify_remote_via_evtchn(__intf->levtchn);

	/* Now we are waiting for the answer from vbstore */
	DBG("Now, we wait for the reply / msg ID: %d (0x%lx)\n", msg.id, &msg.list);

	wait_for_completion(msg.u.reply_wait);

	DBG("Talkv protocol completed / reply: %lx\n", msg.reply);

	/* Consistency check */
	if ((msg.reply->type != msg.type) || (msg.reply->id != msg.id)) {
		printk("%s: reply msg type or ID does not match...\n", __func__);
		printk("VBus received type [%d] expected: %d, received ID [%d] expected: %d\n", msg.reply->type, msg.type, msg.reply->id, msg.id);
		BUG();
	}

	payload = msg.reply->payload;
	if (len != NULL)
		*len = msg.reply->len;

	/* Free the reply msg */
	free(msg.reply);
	free(msg.u.reply_wait);

	mutex_unlock(&vbs_state.request_mutex);

	return payload;
}

/* Send a single message to vbstore.
 * Returns an (heap) allocated message (payload) to be free'd by the called.
 */
static void *vbs_single(struct vbus_transaction t, vbus_msg_type_t type, const char *string, unsigned int *len)
{
	msgvec_t vec;

	vec.base = (void *) string;
	vec.len = strlen(string) + 1;

	return vbs_talkv(t, type, &vec, 1, len);
}

static unsigned int count_strings(const char *strings, unsigned int len)
{
	unsigned int num;
	const char *p;

	for (p = strings, num = 0; p < strings + len; p += strlen(p) + 1)
		num++;

	return num;
}

/* Return the path to dir with /name appended. Buffer must be free()'ed. */
static char *join(const char *dir, const char *name)
{
	char *buffer;

	if (strlen(name) == 0)
		buffer = kasprintf("%s", dir);
	else
		buffer = kasprintf("%s/%s", dir, name);

	return buffer;
}

static char **split(char *strings, unsigned int len, unsigned int *num)
{
	char *p, **ret;

	/* Count the strings. */
	*num = count_strings(strings, len);

	/* Transfer to one big alloc for easy freeing. */
	ret = malloc(*num * sizeof(char *) + len);
	if (!ret)
		BUG();

	memcpy(&ret[*num], strings, len);
	free(strings);

	strings = (char *)&ret[*num];
	for (p = strings, *num = 0; p < strings + len; p += strlen(p) + 1) {
		ret[(*num)++] = p;
	}

	return ret;
}

char **vbus_directory(struct vbus_transaction t, const char *dir, const char *node, unsigned int *num)
{
	char *strings, *path;
	unsigned int len;
	char **rr;

	path = join(dir, node);
	if (IS_ERR(path))
		BUG();

	strings = vbs_single(t, VBS_DIRECTORY, path, &len);
	free(path);

	if (IS_ERR(strings))
		BUG();

	/*
	 * If the result is empty, no allocation has been done and there is no need fo kfree'd anything.
	 */
	if (len) {
		rr = split(strings, len, num);
		return rr;
	} else
		BUG();

	return NULL; /* To make gcc happy :-) */

}

/*
 * Check if a directory exists.
 * Return 1 if the directory has been found, 0 otherwise.
 */
int vbus_directory_exists(struct vbus_transaction t, const char *dir, const char *node) {
	char *strings, *path;
	unsigned int len;
	int res;

	path = join(dir, node);
	if (IS_ERR(path))
		BUG();

	strings = vbs_single(t, VBS_DIRECTORY_EXISTS, path, &len);
	free(path);

	res = (strings[0] == '1') ? 1 : 0;

	/* Don't forget to free the payload */
	free(strings);

	return res;
}

/* Get the value of a single file.
 * Returns a kmalloced value: call free() on it after use.
 * len indicates length in bytes.
 */
void *vbus_read(struct vbus_transaction t, const char *dir, const char *node, unsigned int *len)
{
	char *path;
	void *ret;

	path = join(dir, node);

	ret = vbs_single(t, VBS_READ, path, len);

	free(path);

	return ret;
}

/* Write the value of a single file.
 * Returns -err on failure.
 */
void vbus_write(struct vbus_transaction t, const char *dir, const char *node, const char *string)
{
	char *path;
	msgvec_t vec[2];
	char *str;

	path = join(dir, node);
	if (IS_ERR(path))
		BUG();

	vec[0].base = (void *) path;
	vec[0].len = strlen(path) + 1;

	vec[1].base = (void *) string;
	vec[1].len = strlen(string) + 1;

	str = vbs_talkv(t, VBS_WRITE, vec, ARRAY_SIZE(vec), NULL);

	if (IS_ERR(str))
		BUG();
	if (str)
		free(str);

	free(path);
}

/* Create a new directory. */
void vbus_mkdir(struct vbus_transaction t, const char *dir, const char *node)
{
	char *path;
	char *str;

	path = join(dir, node);
	if (IS_ERR(path))
		BUG();

	str = vbs_single(t, VBS_MKDIR, path, NULL);
	if (IS_ERR(str))
		BUG();

	if (str)
		free(str);

	free(path);
}

/* Destroy a file or directory (directories must be empty). */
void vbus_rm(struct vbus_transaction t, const char *dir, const char *node)
{
	char *path;
	char *str;

	path = join(dir, node);
	if (IS_ERR(path))
		BUG();

	str = vbs_single(t, VBS_RM, path, NULL);
	if (IS_ERR(str))
		BUG();
	if (str)
		free(str);

	free(path);
}

/* Start a transaction: changes by others will not be seen during this
 * transaction, and changes will not be visible to others until end.
 */
void vbus_transaction_start(struct vbus_transaction *t)
{
	mutex_lock(&vbs_state.transaction_mutex);

	if (vbs_state.transaction_count == 0)
		mutex_lock(&vbs_state.transaction_group_mutex);

	vbs_state.transaction_count++;

	transactionID++;

	/* We make sure that an overflow does not lead to value 0 */
	if (unlikely(!transactionID))
		transactionID++;

	DBG("Starting transaction ID: %d...\n", transactionID);
	t->id = transactionID;

	mutex_unlock(&vbs_state.transaction_mutex);
}

/*
 * End a transaction.
 * At this moment, pending watch events raised during operations bound to this transaction ID can be sent to watchers.
 */
void vbus_transaction_end(struct vbus_transaction t)
{
	char *str;

	str = vbs_single(t, VBS_TRANSACTION_END, "", NULL);

	if (IS_ERR(str))
		BUG();

	DBG("Ending transaction ID: %d\n", t.id);

	if (str)
		free(str);
	
	mutex_lock(&vbs_state.transaction_mutex);

	vbs_state.transaction_count--;

	if (vbs_state.transaction_count == 0)
		mutex_unlock(&vbs_state.transaction_group_mutex);

	mutex_unlock(&vbs_state.transaction_mutex);
	
}

/* Single read and scanf: returns -errno or num scanned. */
int vbus_scanf(struct vbus_transaction t, const char *dir, const char *node, const char *fmt, ...)
{
	va_list ap;
	int ret;
	char *val;

	DBG("%s(%s, %s)\n", __func__, dir, node);

	val = vbus_read(t, dir, node, NULL);
	if (IS_ERR(val))
		return PTR_ERR(val);

	va_start(ap, fmt);
	ret = vsscanf(val, fmt, ap);
	va_end(ap);
	free(val);

	/* Distinctive errno. */
	if (ret == 0) {
		printk("%s: ret=0 for %s, %s\n", __func__, dir, node);
		BUG();
	}

	return ret;
}

/* Single printf and write: returns -errno or 0. */
void vbus_printf(struct vbus_transaction t, const char *dir, const char *node, const char *fmt, ...)
{
	va_list ap;
	int ret;
	char *printf_buffer;
	printf_buffer = malloc(PRINTF_BUFFER_SIZE);
	if (printf_buffer == NULL)
		BUG();

	va_start(ap, fmt);
	ret = vsnprintf(printf_buffer, PRINTF_BUFFER_SIZE, fmt, ap);
	va_end(ap);

	BUG_ON(ret > PRINTF_BUFFER_SIZE-1);

	vbus_write(t, dir, node, printf_buffer);

	free(printf_buffer);
}

/* Takes tuples of names, scanf-style args, and void **, NULL terminated.
 * Returns true if all entries have been found, false if at least one entry is not available
 */
bool vbus_gather(struct vbus_transaction t, const char *dir, ...)
{
	va_list ap;
	const char *name;

	va_start(ap, dir);
	while ((name = va_arg(ap, char *)) != NULL) {

		const char *fmt = va_arg(ap, char *);
		void *result = va_arg(ap, void *);
		char *p;

		p = vbus_read(t, dir, name, NULL);
		if (IS_ERR(p))
			BUG();

		/* Case if len == 0, meaning that the entry has not been found in vbstore for instance. */
		if (!strlen(p))
			return false;

		if (sscanf(p, fmt, result) == 0)
			BUG();

		free(p);

	}
	va_end(ap);

	return true;
}

void vbs_watch(const char *path)
{
	msgvec_t vec[1];

	vec[0].base = (void *) path;
	vec[0].len = strlen(path) + 1;

	if (IS_ERR(vbs_talkv(VBT_NIL, VBS_WATCH, vec, ARRAY_SIZE(vec), NULL)))
		BUG();
}

static void vbs_unwatch(const char *path)
{
	msgvec_t vec[1];

	vec[0].base = (char *) path;
	vec[0].len = strlen(path) + 1;

	if (IS_ERR(vbs_talkv(VBT_NIL, VBS_UNWATCH, vec, ARRAY_SIZE(vec), NULL)))
		BUG();
}

/*
 * Look for an existing watch on a certain node (path). The first one is retrieved.
 * Since there might be several watches with different callbacks and the same node, the
 * first watch entry is returned.
 */
static struct vbus_watch *find_first_watch(struct list_head *__watches, const char *node)
{
	struct vbus_watch *__w;

	list_for_each_entry(__w, __watches, list)
		if (!strcmp(__w->node, node))
			return __w;

	return NULL;
}

/*
 * Look for a precise watch in the list of watches. If the watch exists, it is returned (same pointer as the argument)
 * or NULL if it does not exist.
 */
static struct vbus_watch *get_watch(struct list_head *__watches, struct vbus_watch *w)
{
	struct vbus_watch *__w;

	list_for_each_entry(__w, __watches, list)
		if (!strcmp(__w->node, w->node) && (__w->callback == w->callback))
			return __w;

	return NULL;
}

void __register_vbus_watch(struct list_head *__watches, struct vbus_watch *watch) {
	unsigned long flags;

	/* A watch on a certain node with a certain callback has to be UNIQUE. */
	BUG_ON((get_watch(__watches, watch) != NULL));

	if (!find_first_watch(__watches, watch->node))
		vbs_watch(watch->node);
	else
		BUG();

	/* Prepare to update the list */

	flags = local_irq_save();

	watch->pending = false;

	list_add(&watch->list, __watches);

	local_irq_restore(flags);
}


/* Register a watch on a vbstore node */
void register_vbus_watch(struct vbus_watch *watch)
{
	__register_vbus_watch(&watches, watch);
}

static void ____unregister_vbus_watch(struct list_head *__watches, struct vbus_watch *watch, int vbus) {
	unsigned long flags;

	/* Prevent some undesired operations on the list */

	flags = local_irq_save();

	BUG_ON(!get_watch(__watches, watch));

	list_del(&watch->list);

	/* Check if we can remove the watch from vbstore if there is no associated watch
	 * (with the corresponding node name).
	 */
	if (vbus && !find_first_watch(__watches, watch->node)) {

		local_irq_restore(flags);

		vbs_unwatch(watch->node);

	} else
		local_irq_restore(flags);
}


static void __unregister_vbus_watch(struct vbus_watch *watch, int vbus)
{
	____unregister_vbus_watch(&watches, watch, vbus);
}

void unregister_vbus_watch(struct vbus_watch *watch) {
	__unregister_vbus_watch(watch, 1);
}

void unregister_vbus_watch_without_vbus(struct vbus_watch *watch) {
	__unregister_vbus_watch(watch, 0);
}


/*
 * Main threaded function to monitor watches on vbstore entries. Various callbacks can be associated to an event path which is notified
 * by an event message. The pair <event message/callback> has to be unique.
 *
 * The following conditions are handled:
 *
 * IRQs are disabled during manipulations on event message and watch list. Interrupt routine could be executed during a callback operation and
 * some processed event messages may be removed.
 *
 * Secondly, a callback operation might decide to unregister some watches. The watch list has to remain consistent as well.
 *
 */
static void *vbus_watch_thread(void *unused)
{
	struct vbus_watch *__w;
	bool found;

	for (;;) {

		wait_for_completion(&vbs_state.watch_wait);

		/* Avoiding to be suspended during SOO callback processing... */
		mutex_lock(&vbs_state.watch_mutex);

		do {
			found = false;

			list_for_each_entry(__w, &watches, list) {

				/* Is the watch is pending ? */
				if (__w->pending) {

					found = true;
					__w->pending = false;

					/* Execute the watch handler */
					__w->callback(__w);

					/* We go out of loop since a callback operation could manipulate the watch list */
					break;
				}
			}
		} while (found);

		mutex_unlock(&vbs_state.watch_mutex);
	}

	return NULL;
}

irq_return_t vbus_vbstore_isr(int irq, void *data)
{
	vbus_msg_t *msg, *orig_msg, *orig_msg_tmp;
	struct vbus_watch *__w;
	bool found;

	DBG("IRQ: %d intf: %lx req_cons: %d  req_prod: %d\n", irq, __intf, __intf->req_cons, __intf->req_prod);
	DBG("IRQ: %d intf: %lx rsp_cons: %d  rsp_prod: %d\n", irq, __intf, __intf->rsp_cons, __intf->rsp_prod);

	BUG_ON(local_irq_is_enabled());

	while (__intf->rsp_cons != __intf->rsp_prod) {

		msg = malloc(sizeof(vbus_msg_t));
		if (msg == NULL)
			BUG();

		memset(msg, 0, sizeof(vbus_msg_t));

		/* Read the vbus_msg to process */
		vbs_read(msg, sizeof(vbus_msg_t));

		if (msg->len > 0) {
			msg->payload = malloc(msg->len);
			if (msg->payload == NULL)
				BUG();

			memset(msg->payload, 0, msg->len);

			vbs_read(msg->payload, msg->len);

		} else
			/* Override previous value */
			msg->payload = NULL;

		if (msg->type == VBS_WATCH_EVENT) {

			found = false;

			list_for_each_entry(__w, &watches, list) {

				if (!strcmp(__w->node, msg->payload)) {

					__w->pending = true;
					found = true;
				}
			}

			if (found)
				complete(&vbs_state.watch_wait);

			free(msg->payload);
			free(msg);

		} else {

			/* Look for the peer vbus_msg which did the request */
			found = false;

			list_for_each_entry_safe(orig_msg, orig_msg_tmp, &vbus_msg_standby_list, list) {
				if (orig_msg->id == msg->id) {
					list_del(&orig_msg->list); /* Remove this message from the standby list */

					found = true;
					orig_msg->reply = msg;

					/* Wake up the thread waiting for the answer */
					complete(orig_msg->u.reply_wait);

					break; /* Ending the search loop */
				}
			}

			if (!found) /* A pending message MUST exist */
				BUG();

		}

	}

	BUG_ON(local_irq_is_enabled());

	return IRQ_COMPLETED;
}


static void transaction_suspend(void)
{
	mutex_lock(&vbs_state.transaction_mutex);

	/*
	 * From here, no further transaction can be started. We are waiting
	 * for the ongoing transactions to be finished.
	 */
	mutex_lock(&vbs_state.transaction_group_mutex);
}

static void transaction_resume(void)
{
	mutex_unlock(&vbs_state.transaction_group_mutex);
	mutex_unlock(&vbs_state.transaction_mutex);
}

void vbs_suspend(void)
{
	mutex_lock(&vbs_state.watch_mutex);
	mutex_lock(&vbs_state.request_mutex);
	transaction_suspend();
}

void vbs_resume(void)
{
	transaction_resume();

	mutex_unlock(&vbs_state.request_mutex);
	mutex_unlock(&vbs_state.watch_mutex);
}

void vbus_vbstore_init(void)
{
	tcb_t *task;
	struct vbus_device dev;
	uint32_t evtchn;
	int vbus_irq;

	/*
	 * Bind evtchn for interdomain communication: must be executed from the agency or from a ME.
	 */

	/* dev temporary used to set up event channel used by vbstore. */

	dev.otherend_id = 0;
	DBG("%s: binding a local event channel to the remote evtchn %d in Agency (intf: %lx) ...\n", __func__, __intf->revtchn, __intf);

	vbus_bind_evtchn(&dev, __intf->revtchn, &evtchn);

	/* This is our local event channel */
	__intf->levtchn = evtchn;

	DBG("Local vbstore_evtchn is %d (remote is %d)\n", __intf->levtchn, __intf->revtchn);

	/*
	 * Save the (local) levtchn event channel used to communicate with vbstore.
	 * This evtchn will be rebound after migration with the new agency.
	 */
	__vbstore_levtchn = __intf->levtchn;

	INIT_LIST_HEAD(&vbs_state.reply_list);

	mutex_init(&vbs_state.request_mutex);
	mutex_init(&vbs_state.transaction_mutex);
	mutex_init(&vbs_state.watch_mutex);
	mutex_init(&vbs_state.transaction_group_mutex);

	spin_lock_init(&vbs_state.msg_list_lock);

	vbs_state.transaction_count = 0;

	init_completion(&vbs_state.watch_wait);

	/* Initialize the shared memory rings to talk to vbstore */
	vbus_irq = bind_evtchn_to_irq_handler(__intf->levtchn, vbus_vbstore_isr, NULL, NULL);
	if (vbus_irq < 0) {
		printk("VBus request irq failed %i\n", vbus_irq);
		BUG();
	}

	task = kernel_thread(vbus_watch_thread, "vbswatch", NULL, 0);
	if (!task)
		BUG();

	DBG("vbs_init OK!\n");
}

void postmig_vbstore_setup(struct DOMCALL_sync_domain_interactions_args *args) {
	DBG("__vbstore_levtchn=%d\n", __vbstore_levtchn);

	/* Re-assign the levtchn in the intf shared page (seen by the agency too) */
	__intf->levtchn = __vbstore_levtchn;

	DBG("__intf->levtchn=%d\n", __intf->levtchn);

	/* And pass the levtchn to avz for re-binding the existing event channel. */
	args->vbstore_levtchn = __vbstore_levtchn;
}

