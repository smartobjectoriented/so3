/*
 * Copyright (C) 2014-2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <memory.h>
#include <heap.h>
#include <initcall.h>
#include <syscall.h>
#include <banner.h>
#include <timer.h>
#include <softirq.h>
#include <log.h>

#include <asm/cacheflush.h>
#include <asm/mmu.h>
#include <asm/setup.h>

#include <soo/hypervisor.h>
#include <soo/avz.h>
#include <soo/evtchn.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/gnttab.h>

#include <device/arch/gic.h>

#include <avz/uapi/avz.h>

volatile avz_shared_t *avz_shared;

/**
  * @brief Signal-like handler called from AVZ during the recovering operation
  * 
  */
static void resume_fn(void)
{
	u64 time_offset;

	/* Init the local GIC */
	gic_hw_reset();

	/* Init the timer too */
	clocksource_timer_reset();

	/* Now, we need to update the list of all existing timers since our
	 * host clock will be different. We stored the current system time when saving,
	 * so we compute the offset according to the current time.
	 */
	time_offset = NOW() - avz_shared->current_s_time;
	apply_timer_offset(time_offset);

	raise_softirq(SCHEDULE_SOFTIRQ);

	/* We finished with the signal handler processing */
	avz_sig_terminate();
}

/**
 * This function is called at early bootstrap stage along head.S.
 */
void avz_setup(void)
{
	avz_get_shared();

	avz_shared->dom_desc.u.S3C.resume_fn = resume_fn;

	LOG_INFO("SOO Virtualizer (avz) shared page:\n\n");

	LOG_INFO("- Dom phys offset: %lx\n\n", (addr_t) mem_info.phys_base);

	virq_init();

	/* Check that CONFIG_NR_CPUS is correctly set even if we are not SMP in SO3. */
	if (CONFIG_NR_CPUS < 4)
		panic("!! CONFIG_NR_CPUS must be at least equal to 4 !!");
}

void post_init_setup(void)
{
	LOG_INFO("Mapping VBstore shared page pfn %lx\n", avz_shared->dom_desc.u.S3C.vbstore_pfn);

	__intf = (void *) io_map(pfn_to_phys(avz_shared->dom_desc.u.S3C.vbstore_pfn), PAGE_SIZE);
	BUG_ON(!__intf);

	/*
	 * io_map() uses Device-nGnRnE memory attributes (Stage-1). The vbstore
	 * ring buffer is shared with the agency which maps it as Normal cacheable.
	 * Both Stage-1 and Stage-2 must be Normal so the combined effective
	 * memory type is Normal cacheable, enabling hardware cache coherency
	 * (inner-shareable) for the ring buffer protocol.
	 * Override the Stage-1 PTE with Normal cacheable attributes.
	 */
	create_mapping(NULL, (addr_t) __intf, pfn_to_phys(avz_shared->dom_desc.u.S3C.vbstore_pfn), PAGE_SIZE, false);

	LOG_INFO("SOO SO3 capsule booting ...\n");

	soo_guest_activity_init();

	callbacks_init();

	/* Initialize the Vbus subsystem */
	vbus_init();

	/* Write the entries related to the capsule ID in vbstore */
	vbstore_S3C_ID_populate();

	/* How create all vbstore entries required by the frontend drivers */
	vbstore_init_dev_populate();

	LOG_INFO("%s", SO3_BANNER);

	LOG_DEBUG("capsule running as domain %d\n", S3C_domID());
}

REGISTER_POSTINIT(post_init_setup)
