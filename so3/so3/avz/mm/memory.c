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

#include <avz/sched.h>

#define S3C_MEMCHUNK_SIZE 2 * 1024 * 1024
#define S3C_MEMCHUNK_NR 256 /* 256 chunks of 2 MB */

/*
 * Set of memslots in the RAM memory (do not confuse with memchunk !)
 * In the memslot table, the index 0 is for AVZ, the index 1 is for the agency domain (domain 0)
 * and the indexes 2..MEMSLOT_NR are for the MEs. If the S3C_slotID is provided, the index is given by S3C_slotID.
 * Hence, the S3C_slotID matches with the capsule domID.
 */
memslot_entry_t memslot[MEMSLOT_NR];

/* Memory chunks bitmap for allocating MEs */
/* 8 bits per int int */
unsigned int memchunk_bitmap[S3C_MEMCHUNK_NR / 32];

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
 * Number of memchunks the capsule pool may really use: the static pool
 * (S3C_MEMCHUNK_NR) bounded by the configured pool size and by the end of
 * the platform RAM. Without these clamps, allocating one capsule too many
 * silently maps a slot past the end of RAM (QEMU virt) or into a firmware
 * carve-out AVZ cannot see (VideoCore memory on rpi4), and the injection
 * memset destroys it.
 */
static unsigned int s3c_memchunk_avail(void)
{
	addr_t chunk_base = memslot[MEMSLOT_AGENCY].base_paddr + memslot[MEMSLOT_AGENCY].size;
	addr_t ram_end = memslot[MEMSLOT_AVZ].base_paddr + memslot[MEMSLOT_AVZ].size;
	unsigned int avail = S3C_MEMCHUNK_NR;

	if (avail > (unsigned int) (CONFIG_S3C_POOL_SIZE_MB * SZ_1M / S3C_MEMCHUNK_SIZE))
		avail = CONFIG_S3C_POOL_SIZE_MB * SZ_1M / S3C_MEMCHUNK_SIZE;

	if (chunk_base + (addr_t) avail * S3C_MEMCHUNK_SIZE > ram_end)
		avail = (ram_end > chunk_base) ? (ram_end - chunk_base) / S3C_MEMCHUNK_SIZE : 0;

	return avail;
}

/*
 * Allocate a memory slot which satisfies the request.
 *
 * Returns the physical start address or 0 if no slot available.
 */
static unsigned int allocate_memslot(unsigned int order)
{
	int pos;

	pos = bitmap_find_free_region((unsigned long *) &memchunk_bitmap, s3c_memchunk_avail(), order);
	if (pos < 0)
		return 0;

	return memslot[1].base_paddr + memslot[1].size + pos * S3C_MEMCHUNK_SIZE;
}

static void release_memslot(unsigned int addr, unsigned int order)
{
	int pos;

	pos = addr - memslot[1].base_paddr - memslot[1].size;
	pos /= S3C_MEMCHUNK_SIZE;

	bitmap_release_region((unsigned long *) &memchunk_bitmap, pos, order);
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

	mmu_get_current_domain_pgtable(&current_pgtable_paddr);

	if (current_pgtable_paddr == d->pagetable_paddr)
		/* Check if the current page table is identical to the next one. */
		return;

	set_current_domain(d);

	__mmu_switch_kernel((void *) d->pagetable_paddr, true);
}

/**
 * Get the next available memory slot for capsule hosting.
 *
 * @param size		Requested size
 * @param S3C_state	Initial state of the capsule
 * @param slotID	if different than -1, try to allocate to this specific slot 
 * @return int		-1 if no slot is available or <slotID> if a slot is available
 * 
 */
int get_S3C_free_slot(unsigned int size, int slotID)
{
	unsigned int order, addr;
	unsigned int bits_NR;

	/* Do we expect to load into a specific slot? */
	if (slotID == -1) {
		/* Check for available slot */
		for (slotID = MEMSLOT_BASE; slotID < MEMSLOT_NR; slotID++)
			if (!memslot[slotID].busy)
				break;

		if (slotID == MEMSLOT_NR)
			return -1;

	} else if ((slotID < MEMSLOT_BASE) || (slotID >= MEMSLOT_NR) || memslot[slotID].busy)
		return -1;

	/* memslot[slotID] is available */

	bits_NR = DIV_ROUND_UP(size, S3C_MEMCHUNK_SIZE);

	order = get_power_from_size(bits_NR);

	addr = allocate_memslot(order);

	if (!addr)
		return -1; /* No available memory */

	/* Determine the phys/virt start addresses of the guest */

	memslot[slotID].base_paddr = addr;
	memslot[slotID].base_vaddr = S3C_BASE + ((addr_t) (slotID - 1) << S3C_ID_SHIFT);

	memslot[slotID].size = (1 << order) * S3C_MEMCHUNK_SIZE; /* Readjust size */
	memslot[slotID].busy = true;

	/* Map the L2 virtual address space of capsule #(slotID-1) to the physical RAM */
	create_mapping(NULL, memslot[slotID].base_vaddr, memslot[slotID].base_paddr, memslot[slotID].size, false);

	/* Create a domain context including the capsule descriptor before the capsule gets injected. */
	domains[slotID] = domain_create(slotID, S3C_CPU);

	return slotID;
}

/*
 * Release a slot
 */
void put_S3C_slot(unsigned int slotID)
{
	release_mapping(NULL, memslot[slotID].base_vaddr, memslot[slotID].size);

	/* Release the allocated memchunks */
	release_memslot(memslot[slotID].base_paddr, get_power_from_size(DIV_ROUND_UP(memslot[slotID].size, S3C_MEMCHUNK_SIZE)));

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
			printk("%02x ", *((unsigned char *) __xva(MEMSLOT_AVZ, addr)));
			addr++;
		}
		printk("\n");
	}
}

void memslot_init(void)
{
	memset(memslot, 0, sizeof(memslot));
}
