/*
 * Copyright (C) 2026 REDS Institute from HEIG-VD <daniel.rossier@heig-vd.ch>
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

/*
 * Capsule lifecycle callbacks (see s3c_lifecycle_t).
 *
 * The agency calls the capsule at fixed points of its lifecycle; the capsule
 * application decides, through its cb_*() hooks. An application written in
 * user space takes the callbacks over with /dev/soo: it waits for the next
 * one, handles it and answers within LIFECYCLE_USER_DEADLINE, after which the
 * capsule answers with no decision.
 */

#include <common.h>
#include <delay.h>
#include <errno.h>
#include <heap.h>
#include <memory.h>
#include <timer.h>
#include <completion.h>
#include <softirq.h>
#include <schedule.h>
#include <thread.h>
#include <vfs.h>

#include <asm/arm_timer.h>

#include <device/device.h>

#include <soo/avz.h>
#include <soo/soo.h>
#include <soo/lifecycle.h>

#define LIFECYCLE_USER_DEADLINE SECONDS(2)

/* Kernel addresses of the export and inbox buffers, in the snapshot like the rest of the capsule */
static void *export_buf, *inbox_buf;

static completion_t request_ready;
static volatile dc_event_t pending_event;
static volatile bool answered;
static bool user_handler;
static tcb_t *handler_tcb;

/*
 * From its arrival until its post_activate, the capsule's application is
 * held: a user thread that blocks is not scheduled again, so nothing new
 * starts before pre_activate and cooperate have decided. A thread the snapshot
 * caught in the middle of a step finishes it, and releases what the handler
 * may need. The handler itself is never held.
 *
 * A restored capsule is held from its first instruction; at the first boot,
 * from the moment its application takes the callbacks over.
 */
static bool activated, holding;

static s3c_lifecycle_t *lc(void)
{
	return (s3c_lifecycle_t *) &avz_shared->lifecycle;
}

/*
 * Hand a callback over to the user-space handler, if there is one, and wait
 * for its answer. At the arrival of the capsule, the handler runs before it
 * is activated: its devices, the console included, are not connected yet.
 */
void lifecycle_user(dc_event_t dc_event)
{
	u64 deadline;

	/* At the first boot, the application may not have taken the callbacks over yet. */

	deadline = NOW() + LIFECYCLE_USER_DEADLINE;
	while (!user_handler && !activated && (dc_event == DC_PRE_ACTIVATE) && (NOW() < deadline))
		msleep(10);

	if (!user_handler)
		return;

	answered = false;
	pending_event = dc_event;
	complete(&request_ready);

	deadline = NOW() + LIFECYCLE_USER_DEADLINE;
	while (!answered && (NOW() < deadline))
		msleep(10);

	if (!answered)
		lprintk("%s: no answer to lifecycle event %d, no decision taken\n", __func__, dc_event);

	pending_event = DC_NO_EVENT;
}

__attribute__((weak)) void cb_pre_activate(s3c_lifecycle_t *lifecycle)
{
	lifecycle_user(DC_PRE_ACTIVATE);
}

__attribute__((weak)) void cb_cooperate(s3c_lifecycle_t *lifecycle)
{
	lifecycle_user(DC_COOPERATE);
}

__attribute__((weak)) void cb_cooperate_cb(s3c_lifecycle_t *lifecycle)
{
	lifecycle_user(DC_COOPERATE_CB);
}

__attribute__((weak)) void cb_cooperate_remote(s3c_lifecycle_t *lifecycle)
{
	lifecycle_user(DC_COOPERATE_REMOTE);
}

bool lifecycle_held(tcb_t *tcb)
{
	return holding && tcb->pcb && tcb->blocked && !tcb->killed && (tcb != handler_tcb);
}

/* Called first thing after a restore, before any thread runs. */
void lifecycle_restored(void)
{
	holding = true;
}

/* At post_activate: the application runs. */
void lifecycle_activated(void)
{
	activated = true;
	holding = false;
	raise_softirq(SCHEDULE_SOFTIRQ);
}

/* Called on a lifecycle event from the agency, before it gets its answer. */
void lifecycle_callback(dc_event_t dc_event)
{
	s3c_lc_out_t *out = &lc()->out;

	/*
	 * A decision holds for the call it answers; stop_propagate holds for
	 * good, and a request to cooperate until a cooperate callback serves it.
	 */

	out->flags &= S3C_LC_STOP_PROPAGATE | (dc_event == DC_COOPERATE ? 0 : S3C_LC_COOPERATE);
	out->kill_mask = 0;
	out->coop_mask = 0;
	out->export_len = 0;

	switch (dc_event) {
	case DC_PRE_ACTIVATE:
		cb_pre_activate(lc());
		break;
	case DC_COOPERATE:
		cb_cooperate(lc());
		break;
	case DC_COOPERATE_CB:
		cb_cooperate_cb(lc());
		break;
	case DC_COOPERATE_REMOTE:
		cb_cooperate_remote(lc());
		break;
	default:
		break;
	}
}

static int soo_ioctl(int fd, unsigned long cmd, unsigned long args)
{
	s3c_lc_out_t *out = &lc()->out;
	struct soo_callback *cb;
	struct soo_answer *answer;
	struct soo_ids *ids;

	switch (cmd) {
	case SOO_IOC_WAIT:
		cb = (struct soo_callback *) args;
		user_handler = true;
		handler_tcb = current();
		if (!activated)
			holding = true;

		do {
			if (wait_for_completion_interruptible(&request_ready))
				return -EINTR;
		} while (pending_event == DC_NO_EVENT);

		cb->event = pending_event;
		cb->in = lc()->in;
		return 0;

	case SOO_IOC_ANSWER:
		if (pending_event == DC_NO_EVENT)
			return -EINVAL;

		answer = (struct soo_answer *) args;
		out->flags |= answer->flags & (S3C_LC_KILL_SELF | S3C_LC_SKIP_ACTIVATION | S3C_LC_STOP_PROPAGATE);
		out->kill_mask = answer->kill_mask;
		out->coop_mask = answer->coop_mask;
		answered = true;
		return 0;

	case SOO_IOC_SET_SUMMARY:
		memcpy(out->summary, (void *) args, S3C_LC_SUMMARY_LEN);
		return 0;

	case SOO_IOC_STOP_PROPAGATE:
		out->flags |= S3C_LC_STOP_PROPAGATE;
		return 0;

	case SOO_IOC_COOPERATE:
		out->flags |= S3C_LC_COOPERATE;
		return 0;

	case SOO_IOC_GET_IDS:
		ids = (struct soo_ids *) args;
		ids->capsule_uid = out->capsule_uid;
		ids->spid = get_S3C_desc()->spid;
		ids->slotID = S3C_domID();
		ids->device_uid = lc()->in.device_uid;
		memcpy(ids->devices, lc()->in.devices, S3C_LC_DEVICES_LEN);
		return 0;

	default:
		return -EINVAL;
	}
}

/* The data of the capsule cooperating with us, in a cooperate_cb callback. */
static int soo_read(int fd, void *buffer, int count)
{
	uint32_t len = lc()->in.inbox_len;

	if (len > S3C_LC_BUF_SIZE)
		len = S3C_LC_BUF_SIZE;
	if (count > len)
		count = len;

	memcpy(buffer, inbox_buf, count);

	return count;
}

/* Our data for the capsules we cooperate with. */
static int soo_write(int fd, const void *buffer, int count)
{
	if (count > S3C_LC_BUF_SIZE)
		count = S3C_LC_BUF_SIZE;

	memcpy(export_buf, buffer, count);
	lc()->out.export_len = count;

	return count;
}

static struct file_operations soo_fops = {
	.ioctl = soo_ioctl,
	.read = soo_read,
	.write = soo_write,
};

static struct devclass soo_cdev = {
	.class = "soo",
	.type = VFS_TYPE_DEV_CHAR,
	.fops = &soo_fops,
};

/*
 * At the first boot of the capsule only: a restored capsule finds its area,
 * ID and buffers in its snapshot.
 */
void lifecycle_init(void)
{
	s3c_lc_out_t *out = &lc()->out;
	addr_t buf;

	init_completion(&request_ready);
	devclass_register(NULL, &soo_cdev);

	/* The agency described the device before we booted: keep that. */

	memset(out, 0, sizeof(*out));

	/* Unique enough across devices: the counter is never the same twice, nor on two devices at once. */

	out->capsule_uid = (arch_counter_get_cntvct() << 8) ^ ((u64) S3C_domID() << 56) ^ get_S3C_desc()->spid;
	if (!out->capsule_uid)
		out->capsule_uid = 1;

	buf = get_contig_free_vpages(2 * S3C_LC_BUF_SIZE / PAGE_SIZE);
	BUG_ON(!buf);

	export_buf = (void *) buf;
	inbox_buf = (void *) (buf + S3C_LC_BUF_SIZE);
	out->export_paddr = virt_to_phys_pt(buf);
	out->inbox_paddr = virt_to_phys_pt(buf + S3C_LC_BUF_SIZE);
}
