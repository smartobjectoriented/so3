/*
 * Copyright (C) 2014-2021 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#include <common.h>
#include <types.h>
#include <memory.h>
#include <spinlock.h>
#include <heap.h>
#include <bitmap.h>
#include <string.h>

#include <avz/sched.h>

#define ME_MEMCHUNK_SIZE	2 * 1024 * 1024
#define ME_MEMCHUNK_NR		256    /* 256 chunks of 2 MB */

/*
 * Set of memslots in the RAM memory (do not confuse with memchunk !)
 * In the memslot table, the index 0 is for AVZ, the index 1 is for the two agency domains (domain 0 (non-RT) and domain 1 (RT))
 * and the indexes 2..MEMSLOT_NR are for the MEs. If the ME_slotID is provided, the index is given by ME_slotID.
 * Hence, the ME_slotID matches with the ME domID.
 */
memslot_entry_t memslot[MEMSLOT_NR];

/* Memory chunks bitmap for allocating MEs */
/* 8 bits per int int */
unsigned int memchunk_bitmap[ME_MEMCHUNK_NR/32];

/*
 * Returns the power of 2 (order) which matches the size
 */
unsigned int get_power_from_size(unsigned int bits_NR) {
	unsigned int order;

	/* Find the power of 2 which matches the number of bits */
	order = -1;

	do {
		bits_NR = bits_NR >> 1;
		order++;
	} while (bits_NR);

	return order;
}

/*
 * Allocate a memory slot which satisfies the request.
 *
 * Returns the physical start address or 0 if no slot available.
 */
unsigned int allocate_memslot(unsigned int order) {
	int pos;

	pos = bitmap_find_free_region((unsigned long *) &memchunk_bitmap, ME_MEMCHUNK_NR, order);
	if (pos < 0)
		return 0;

#ifdef DEBUG
	printk("allocate_memslot param %d\n", order);
	printk("allocate_memslot pos %d\n", pos);
	printk("allocate_memslot memslot1start %08x\n", (unsigned int) memslot[1].base_paddr);
	printk("allocate_memslot memslot1size %d\n", memslot[1].size);
	printk("allocate_memslot pos*MEMCHUNK_SIZE %d\n", pos*ME_MEMCHUNK_SIZE);
#endif

	return memslot[1].base_paddr + memslot[1].size + pos*ME_MEMCHUNK_SIZE;
}

void release_memslot(unsigned int addr, unsigned int order) {
	int pos;

	pos = addr - memslot[1].base_paddr - memslot[1].size;
	pos /= ME_MEMCHUNK_SIZE;

#ifdef DEBUG
	printk("release_memslot addr %08x\n", addr);
	printk("release_memslot order %d\n", order);
	printk("release_memslot pos %d\n", pos);
#endif

	bitmap_release_region((unsigned long *) &memchunk_bitmap, pos, order);
}

/*
 * switch_mm_domain() is used to perform a memory context switch between domains.
 * @d refers to the domain
 * @next_addrspace refers to the address space to be considered with this domain.
 * @current_addrspace will point to the current address space.
 */
void switch_mm_domain(struct domain *d) {
	addr_t current_pgtable_paddr;

	mmu_get_current_pgtable(&current_pgtable_paddr);

	if (current_pgtable_paddr == d->avz_shared->pagetable_paddr)
	/* Check if the current page table is identical to the next one. */
		return ;

	set_current_domain(d);

#ifdef CONFIG_ARM64VT
	__mmu_switch_kernel((void *) d->avz_shared->pagetable_paddr, true);
#else
	mmu_switch_kernel((void *) d->avz_shared->pagetable_paddr);
#endif

}

/**
 * Get the next available memory slot for ME hosting.
 *
 * @param size		Requested size
 * @param ME_state	Initial state of the ME
 * @return		-1 if no slot is available or <slotID> if a slot is available.
 */
int get_ME_free_slot(unsigned int size, ME_state_t ME_state) {
	unsigned int order, addr;
	int slotID;
	unsigned int bits_NR;

	/* Check for available slot */
	for (slotID = MEMSLOT_BASE; slotID < MEMSLOT_NR; slotID++)
		if (!memslot[slotID].busy)
			break;

	if (slotID == MEMSLOT_NR)
		return -1;

	/* memslot[slotID] is available */

	bits_NR = DIV_ROUND_UP(size, ME_MEMCHUNK_SIZE);

	order = get_power_from_size(bits_NR);

	addr = allocate_memslot(order);

	if (!addr)
		return -1;  /* No available memory */

	memslot[slotID].base_paddr = addr;
	memslot[slotID].size = (1 << order) * ME_MEMCHUNK_SIZE;  /* Readjust size */
	memslot[slotID].busy = true;

#ifdef DEBUG
	printk("get_ME_slot param %d\n", size);
	printk("get_ME_slot bits_NR %d\n", bits_NR);
	printk("get_ME_slot slotID %d\n", slotID);
	printk("get_ME_slot start %08x\n", (unsigned int) memslot[slotID].base_paddr);
	printk("get_ME_slot size %d\n", memslot[slotID].size);
#endif

	/* Create a domain context including the ME descriptor before the ME gets injected. */

	domains[slotID] = domain_create(slotID, ME_CPU);

	/* Initialize the ME descriptor */
	set_ME_state(slotID, ME_state);

	return slotID;
}

/*
 * Release a slot
 */
void put_ME_slot(unsigned int slotID) {

	/* Release the allocated memchunks */
	release_memslot(memslot[slotID].base_paddr, get_power_from_size(DIV_ROUND_UP(memslot[slotID].size, ME_MEMCHUNK_SIZE)));

	memslot[slotID].busy = false;

#ifdef DEBUG
  printk("put_ME_slot param %d\n", slotID);
#endif
}

void dump_page(unsigned int pfn) {

	int i, j;
	unsigned int addr;

	addr = (pfn << 12);

	printk("%s: pfn %x\n\n", __func__,  pfn);

	for (i = 0; i < PAGE_SIZE; i += 16) {
		printk(" [%x]: ", i);
		for (j = 0; j < 16; j++) {
			printk("%02x ", *((unsigned char *) __lva(addr)));
			addr++;
		}
		printk("\n");
	}
}

void memslot_init(void) {
	memset(memslot, 0, sizeof(memslot));
}

