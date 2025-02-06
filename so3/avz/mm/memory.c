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

#include <asm/mmu.h>

#include <avz/soo.h>

#include <avz/sched.h>

#define ME_MEMCHUNK_SIZE 2 * 1024 * 1024
#define ME_MEMCHUNK_NR 256 /* 256 chunks of 2 MB */

/*
 * Set of memslots in the RAM memory (do not confuse with memchunk !)
 * In the memslot table, the index 0 is for AVZ, the index 1 is for the two agency domains (domain 0 (non-RT) and domain 1 (RT))
 * and the indexes 2..MEMSLOT_NR are for the MEs. If the ME_slotID is provided, the index is given by ME_slotID.
 * Hence, the ME_slotID matches with the ME domID.
 */
memslot_entry_t memslot[MEMSLOT_NR];

/* Memory chunks bitmap for allocating MEs */
/* 8 bits per int int */
unsigned int memchunk_bitmap[ME_MEMCHUNK_NR / 32];

/*
 * Returns the power of 2 (order) which matches the size
 */
unsigned int get_power_from_size(unsigned int bits_NR)
{
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
static unsigned int allocate_memslot(unsigned int order)
{
	int pos;

	pos = bitmap_find_free_region((unsigned long *)&memchunk_bitmap,
				      ME_MEMCHUNK_NR, order);
	if (pos < 0)
		return 0;

	return memslot[1].base_paddr + memslot[1].size + pos * ME_MEMCHUNK_SIZE;
}

static void release_memslot(unsigned int addr, unsigned int order)
{
	int pos;

	pos = addr - memslot[1].base_paddr - memslot[1].size;
	pos /= ME_MEMCHUNK_SIZE;

	bitmap_release_region((unsigned long *)&memchunk_bitmap, pos, order);
}

/*
 * switch_mm_domain() is used to perform a memory context switch between domains.
 * @d refers to the domain
 * @next_addrspace refers to the address space to be considered with this domain.
 * @current_addrspace will point to the current address space.
 */
void switch_mm_domain(struct domain *d)
{
	addr_t current_pgtable_paddr;

	mmu_get_current_pgtable(&current_pgtable_paddr);

	if (current_pgtable_paddr == d->pagetable_paddr)
		/* Check if the current page table is identical to the next one. */
		return;

	set_current_domain(d);

	__mmu_switch_kernel((void *)d->pagetable_paddr, true);
}

/**
 * Get the next available memory slot for ME hosting.
 *
 * @param size		Requested size
 * @param ME_state	Initial state of the ME
 * @return		-1 if no slot is available or <slotID> if a slot is available.
 */
int get_ME_free_slot(unsigned int size)
{
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
		return -1; /* No available memory */

	/* Determine the phys/virt start addresses of the guest */

	memslot[slotID].base_paddr = addr;
	memslot[slotID].base_vaddr =
		ME_BASE + ((addr_t)(slotID - 1) << ME_ID_SHIFT);

	memslot[slotID].size =
		(1 << order) * ME_MEMCHUNK_SIZE; /* Readjust size */
	memslot[slotID].busy = true;

	/* Map the L2 virtual address space of ME #(slotID-1) to the physical RAM */
	create_mapping(NULL, memslot[slotID].base_vaddr,
		       memslot[slotID].base_paddr, memslot[slotID].size, false);

	/* Create a domain context including the ME descriptor before the ME gets injected. */
	domains[slotID] = domain_create(slotID, ME_CPU);

	return slotID;
}

/*
 * Release a slot
 */
void put_ME_slot(unsigned int slotID)
{
	release_mapping(NULL, memslot[slotID].base_vaddr, memslot[slotID].size);

	/* Release the allocated memchunks */
	release_memslot(memslot[slotID].base_paddr,
			get_power_from_size(DIV_ROUND_UP(memslot[slotID].size,
							 ME_MEMCHUNK_SIZE)));

	memslot[slotID].busy = false;
}

void dump_page(unsigned int pfn)
{
	int i, j;
	unsigned int addr;

	addr = (pfn << 12);

	printk("%s: pfn %x\n\n", __func__, pfn);

	for (i = 0; i < PAGE_SIZE; i += 16) {
		printk(" [%x]: ", i);
		for (j = 0; j < 16; j++) {
			printk("%02x ",
			       *((unsigned char *)__xva(MEMSLOT_AVZ, addr)));
			addr++;
		}
		printk("\n");
	}
}

void memslot_init(void)
{
	memset(memslot, 0, sizeof(memslot));
}
