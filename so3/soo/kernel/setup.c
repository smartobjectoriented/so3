/*
 * Copyright (C) 2014-2022 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <asm/cacheflush.h>
#include <asm/mmu.h>
#include <asm/setup.h>

#include <soo/hypervisor.h>
#include <soo/avz.h>
#include <soo/evtchn.h>
#include <soo/soo.h>
#include <soo/console.h>
#include <soo/gnttab.h>

#include <soo/debug/logbool.h>

#include <avz/uapi/avz.h>

extern volatile uint32_t *HYPERVISOR_hypercall_addr;

#ifdef CONFIG_ARCH_ARM32

void *__guestvectors = NULL;

/* Avoid large area on stack (limited to 1024 bytes */

unsigned char vectors_tmp[PAGE_SIZE];

#endif

int do_presetup_adjust_variables(void *arg)
{
	struct DOMCALL_presetup_adjust_variables_args *args = arg;

	/* Normally, avz_start_info virt address is retrieved from r12 at guest bootstrap (head.S)
	 * We need to readjust this address after migration.
	 */
	avz_shared = args->avz_shared;

	avz_guest_phys_offset = avz_shared->dom_phys_offset;

	HYPERVISOR_hypercall_addr = (uint32_t *) avz_shared->hypercall_vaddr;

	__printch = avz_shared->printch;

	/* Adjust timer information */
	postmig_adjust_timer();

	return 0;
}

int do_postsetup_adjust_variables(void *arg)
{
	struct DOMCALL_postsetup_adjust_variables_args *args = arg;

	/* Updating pfns where used. */
	readjust_io_map(args->pfn_offset);

	return 0;
}

int do_sync_domain_interactions(void *arg)
{
	struct DOMCALL_sync_domain_interactions_args *args = arg;

	io_unmap((addr_t) __intf);

	__intf = (struct vbstore_domain_interface *) io_map(args->vbstore_pfn << PAGE_SHIFT, PAGE_SIZE);
	BUG_ON(!__intf);

	postmig_vbstore_setup(args);

	return 0;
}

/**
 * This function is called at early bootstrap stage along head.S.
 */
void avz_setup(void) {

	__printch = avz_shared->printch;

	avz_guest_phys_offset = avz_shared->dom_phys_offset;

	/* Immediately prepare for hypercall processing */
	HYPERVISOR_hypercall_addr = (uint32_t *) avz_shared->hypercall_vaddr;

	lprintk("SOO Virtualizer (avz) shared page:\n\n");

	lprintk("- Virtual address of printch() function: %lx\n", __printch);
	lprintk("- Hypercall addr: %lx\n", (addr_t) HYPERVISOR_hypercall_addr);
	lprintk("- Dom phys offset: %lx\n\n", (addr_t) avz_guest_phys_offset);

	__ht_set = (ht_set_t) avz_shared->logbool_ht_set_addr;

	avz_shared->domcall_vaddr = (unsigned long) domcall;
	avz_shared->vectors_vaddr = (unsigned long) avz_vector_callback;
	avz_shared->traps_vaddr = (unsigned long) trap_handle;

	virq_init();
}


void post_init_setup(void) {

	printk("VBstore shared page with agency at pfn 0x%x\n", avz_shared->vbstore_pfn);

	__intf = (struct vbstore_domain_interface *) io_map(avz_shared->vbstore_pfn << PAGE_SHIFT, PAGE_SIZE);
	BUG_ON(!__intf);

	printk("SOO Mobile Entity booting ...\n");

	soo_guest_activity_init();

	callbacks_init();

	/* Initialize the Vbus subsystem */
	vbus_init();

	gnttab_init();

	/*
	 * Now, the ME requests to be paused by setting its state to ME_state_preparing. As a consequence,
	 * the agency will pause it.
	 * The state is moving from ME_state_booting to ME_state_preparing.
	 */
	set_ME_state(ME_state_preparing);

	/*
	 * There are two scenarios.
	 * 1. Classical injection scheme: Wait for the agency to perform the pause+unpause. It should set the ME
	 *    state to ME_state_booting to allow the ME to continue.
	 * 2. ME that has migrated on a Smart Object: The ME state is ME_state_migrating, so it is different from
	 *    ME_state_preparing.
	 */

	while (1) {
		schedule();

		if (get_ME_state() != ME_state_preparing) {
			DBG("ME state changed: %d, continuing...\n", get_ME_state());
			break;
		}
	}

	BUG_ON(get_ME_state() != ME_state_booting);

	/* Write the entries related to the ME ID in vbstore */
	vbstore_ME_ID_populate();

	/* How create all vbstore entries required by the frontend drivers */
	vbstore_init_dev_populate();

	printk("SO3  Operating System -- Copyright (c) 2016-2022 REDS Institute (HEIG-VD)\n\n");

	DBG("ME running as domain %d\n", ME_domID());
}

#ifdef CONFIG_ARCH_ARM32

void vectors_setup(void) {

 	/* Make a copy of the existing vectors. The L2 pagetable was allocated by AVZ and cannot be used as such by the guest.
 	 * Therefore, we will make our own mapping in the guest for this vector page.
 	 */
 	memcpy(vectors_tmp, (void *) VECTOR_VADDR, PAGE_SIZE);

 	/* Reset the L1 PTE used for the vector page. */
 	clear_l1pte(NULL, VECTOR_VADDR);

 	create_mapping(NULL, VECTOR_VADDR, __pa((uint32_t) __guestvectors), PAGE_SIZE, true);

 	memcpy((void *) VECTOR_VADDR, vectors_tmp, PAGE_SIZE);

	/* We need to add handling of swi/svc software interrupt instruction for syscall processing.
	 * Such an exception is fully processed by the SO3 domain.
	 */
	inject_syscall_vector();

	__asm_flush_dcache_range(VECTOR_VADDR, VECTOR_VADDR + PAGE_SIZE);
	invalidate_icache_all();
}

void pre_irq_init_setup(void) {

	/* Create a private vector page for the guest vectors */
	 __guestvectors = memalign(PAGE_SIZE, PAGE_SIZE);
	BUG_ON(!__guestvectors);

	vectors_setup();
}

REGISTER_PRE_IRQ_INIT(pre_irq_init_setup)

#endif /* CONFIG_ARCH_ARM32 */

REGISTER_POSTINIT(post_init_setup)
